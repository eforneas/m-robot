/*
 * 
 * Autor: Eduardo J. Fórneas Lence - Innovanube de Computación SL
 * 
 * comprobar que un par de BT-05 emparejados envían y reciben datos
 * 
 * el proceso se comprueba en tres pasos:
 * 
 * 1) a través del terminal serie enviamos datos que son leídos por BT_master
 * 2) BT_master envia los datos recibidos a BT_slave
 * 3) BT_slave envía una respuesta recibidos al BT_master
 *
 *  detalles del pineado de la placa (Arduino UNO)
 *  
 *  pin 10 -> al pin BT master (RX)
 *  pin 11 -> al pin BT master (TX)
 *  pin  2 -> al pin BT slave  (TX)
 *  pin  3 -> al pin BT slave  (RX)
 *  
 *  ambos deben tener suministro de corriente y deben estar en modo no programable, una vez terminada la tarea de
 *  emparejamiento, los leds de ambos módulos deben parpadear dos veces cada 2 segundos (aprox.) el módulo master
 *  primero y el slave después nada más aplicar corriente al circuito.
 *
 * Para configurar el BT-05, pulsar el botón del módulo antes de darle tensión (*), esperar un par de segundos
 * y empezará un parpadeo lento, reprogramar usando comandos at, una vez terminado, retirar la tensión, volver a conectar
 * y el led comenzará de nuevo a parpadear rapidamente, los módulos han de ser programados de uno en uno, comenzando por
 * el slave, ya que su addr será necesaria para emparejarlo con el módulo master, que será el último en ser programado
 * una vez terminado la programación, se retirará la tensión de alimentación, se cargará en la plaza el sketch de prueba y se
 * volverá a suministrar tensión a los módulos.
 * 
 * (*) si el módulo carece de pulsador para poner en modo AT2, es posible que deba hacerse con una señal HIGH en el pin STATE,
 * en este caso usar cualquier pin disponible y en setup() poner a HIGH el pin conectado e introducir un delay de un par 
 * de segundos para que el módulo pase a modo AT2 (programación) mientras exista ésta señal el módulo podrá ser programado
 * 
 * *********************************************************************************************************************
 * 
 * para emparejar arduinos:
 * 
 * el esclavo:
 * 
 * ok <¬
 * at+orgl <¬
 * at+rmaad <¬
 * at+role=0
 * at+name=xxxx   // nombre identificativo del esclavo
 * at+pswd=1234 <¬
 * at+addr <¬
 *      1234:56:678901
 *      esta addr en hexadecimal es la que habrá que indicar en el maestro para emparejar ambos bluetooth
 *      tomar nota, porque será necesaria para programar el módulos master
 * 
 * at+init <¬  inicializa el módulo y aplica la configuración dada
 ***********************************************************************************************************************
 *
 * el maestro: 
 * 
 * at <¬
 * at+orgl <¬   // poner el módulo en modo origen
 * at+rmaad <¬  // eliminar los emparejados anteriores
 * at+rmasd <¬  // borra todos los blutooth autenticados
 * at+role=1    // modo maestro
 * at+name=xxxx // dar al módulo un nombre identificativo
 * at+pswd=1234 <¬
 * at+cmode=0 <~ 0 emparejar con un solo módulo bluetooth 1 para emparejar con más de un módulo
 * at+bind=0000,00,000000     dirección mac 48 bits del módulo bluetooth a emparejar (el esclavo) ejemplo: 98d3:32:70c41e
 *                            es posible enviar la mac coneste formato 0000,00,000000 (nap,uap,lap format)
 * at+init                    inicializa el SPP 
 * at+pair=0000,00,000000,5   emparejar con dispositivo, timeout, si da error, eliminar el parámetro de timeout
 * at+link=0000,00,000000     enlazar con el dispositivo emparejado
 * 
 * *********************************************************************************************************************
 * 




*/



#include <SoftwareSerial.h>   // Incluimos la librería  SoftwareSerial  

SoftwareSerial BT_master( 8, 9);      // Definimos los pines RX y TX del Arduino conectados al Bluetooth
SoftwareSerial BT_slave( 6, 7 );         // Definimos los pines RX y TX del Arduino conectados al Bluetooth

const byte numChars = 254;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false ;
char rc ;

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
 
   while (Serial.available() > 0 && newData == false) {
     rc = Serial.read();

     if (rc != endMarker) {
       receivedChars[ndx] = rc;
       ndx++;
       if (ndx >= numChars) {
         ndx = numChars - 1;
       }
      } else {
        receivedChars[ndx] = '\0'; // terminate the string
        ndx = 0;
        newData = true;
      }
    }
}

void showNewData() {
  if (newData == true) {

    Serial.println() ;
    Serial.print( "Sending data: " );
    Serial.println( receivedChars );

    BT_master.print( receivedChars ) ;  // print in master...
    BT_slave.listen() ;                 // ... and listen in slave

    newData = false;

  }
}

 
void setup() {

  pinMode(7, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(6, INPUT);
  pinMode(8, INPUT);

  BT_master.begin( 38400 ) ;  // 
  BT_slave.begin( 38400 ) ;   // last open, last listen

  delay( 100 ) ;
  
  Serial.begin(9600);           // serial port ready
  while (!Serial) {
    ;
  }
  
  Serial.println( "Bluetooth's ready!" ) ;  
}
 
void loop() {

  if ( BT_master.available() > 0 ) {
    Serial.print( "Listen BT master: " );
    while ( BT_master.available() > 0 ) {
      rc = BT_master.read();
      Serial.print( rc );
    }
    Serial.println();
    BT_slave.listen() ; // change port to listen
  }

  if ( BT_slave.available() > 0 ) {
    Serial.print( "Listen BT slave: " );
    while ( BT_slave.available() > 0 ) {
      rc = BT_slave.read();
      Serial.print( rc );
    }
    Serial.println();
    BT_slave.print("Yes Master");   // send something to response
    BT_master.listen() ;            // change port to listen
    
  }

  // capture and send serial from master to slave
  recvWithEndMarker();
  showNewData();

  delay( 50 ) ;

}

