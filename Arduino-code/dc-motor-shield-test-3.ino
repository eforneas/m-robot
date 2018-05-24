/*
 * (C) Innovanube de Computación SL
 * (C) Eduardo Fórneas
 * 
 * Proyect:
 *
 * Develop of a platform for autonomus vehicle guided by computer vision
 * 
 * Hardware:
 *    - Arduino and Rasberry Pi (Arduino for movement and sensors control and Raspberry PI for vision)
 *    - Bluetooth HC-05
 *    - PIR sensor
 *    - Laser Sensors
 *    - IR sensors
 *    - Ultrasonic sensor
 *    - 12 V DC motors
 * 
 * Software:
 *    - Arduino IDE
 *    - Python 2.7
 *    - ROS libraries
 *    - OpenCV
 * 
 * Bluetooth connection using the App: Arduino Bluetooth Joystick by Danius Kalvaitis
 * 
 * Google play download:
 * 
 * https://play.google.com/store/apps/details?id=daniuscompany.bluetoothremotejoystick&hl=es
 * 
 * Thanks to:
 * 
 * Development by: Danius Kalvaitis
 * Contact email: danius.kalvaitis@gmail.com
 * 
 * Motors testing using joysticks emulated by the app
 * Valid for manual guide without camera and other sensors
 * 
 */

#include <AFMotor.h>
#include <SoftwareSerial.h>   // SoftwareSerial library include

/*
    Frequencies for channel 1 & 2 are:
    MOTOR12_64KHZ
    MOTOR12_8KHZ
    MOTOR12_2KHZ
    MOTOR12_1KHZ
    Frequencies for channel 3 & 4 are:
    MOTOR34_64KHZ
    MOTOR34_8KHZ
    MOTOR34_1KHZ
 */

AF_DCMotor motor1(1);   // right front
AF_DCMotor motor2(2);   // right rear
AF_DCMotor motor3(3);   // left front
AF_DCMotor motor4(4);   // left rear

// change if necesary
// 14 = A0 to TXD bluetooth module
// 15 = A1 to RXD bluetooth module
SoftwareSerial BT_master( 14, 15 );      // Define RX and TX Arduino pins for Bluetooth connection

// general debugger
static boolean DEBUG = false;
// movement debugger
static boolean moveDEBUG = false;
// bluetooth debugger
static boolean blueDEBUG = false;

// joystick tolerance value for minimun movement
static byte tolerance = 10;

// joystick ports for testing purpose only
static int joyX = 0;
static int joyY = 1;

// max speed for DC motors
static int MaxSpeed = 255;

// convert real joystick reads to 0 - MaxScale range
static int MaxScale = 1024 ;

// current calculated speed on 0 - MaxSpeed range
int velocity = 0;
// last velocity
int oldVelocity = 0;
// current direction
String oldDirection;

// current turning angle, calculated on Y joystick position
int turn_angle = 0;

// current joystick position
int Xpos = 0;
int Ypos = 0;
// number of button pressed
byte nButton;

// default init values for joystick, aprox. 512 in center position
int Xdef = 0;
int Ydef = 0;

// old joysticks reads
int oldXpos = 0;
int oldYpos = 0;

/*
 * variables used for bluetooth data receiving
 */
// max received bytes from bluetooth connection
const byte numChars = 254;
// array of chars
char receivedChars[numChars]; // an array to store the received data
// read string from bluetooth
String bt_read_result = "";

void set_motors( String go_direction ) {
  
  // forward
  if ( go_direction == "f" && go_direction != oldDirection ) {
    if ( moveDEBUG ) {
      Serial.println("Change motors direction: forward");
    }

    motor1.run( BACKWARD );
    motor2.run( BACKWARD );
    motor3.run( FORWARD );
    motor4.run( FORWARD );

  } else if ( go_direction == "b" && go_direction != oldDirection ) {
    // backward
    if ( moveDEBUG ) {
      Serial.println("Change motors direction: backward");
    }

    motor1.run( FORWARD );
    motor2.run( FORWARD );
    motor3.run( BACKWARD );
    motor4.run( BACKWARD );
     
  } else if ( go_direction == "r" && go_direction != oldDirection ) {
    // turn on right, vehicle stopped
    if ( moveDEBUG ) {
      Serial.println("Change motors direction: turn on right");
    }

    motor1.run( FORWARD );
    motor2.run( FORWARD );
    motor3.run( FORWARD );
    motor4.run( FORWARD );
    
  } else if ( go_direction == "l" && go_direction != oldDirection ) {
    // turn on left, vehicle stopped
    if ( moveDEBUG ) {
      Serial.println("Change motors direction: turn on left");
    }

    motor1.run( BACKWARD );
    motor2.run( BACKWARD );
    motor3.run( BACKWARD );
    motor4.run( BACKWARD );
  }

  // save actual direction as old direction
  oldDirection = go_direction;

}

void set_speed( int new_speed ){
  motor1.setSpeed( new_speed );
  motor2.setSpeed( new_speed );
  motor3.setSpeed( new_speed );
  motor4.setSpeed( new_speed );  
}

void stop_now() {
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);

  velocity = 0;
  oldDirection = "";
}

void setup() {
  Serial.begin(9600);

  /*
   * open connection for bluetooth using hardware module and app
   */
  BT_master.begin( 38400 ) ;  // open serial over bluetooth
  delay( 100 ) ;

  /*
   * use this values for bluetooth and app use
   * read delay from app are in settings itself
   */
  Xdef = 512;
  Ydef = 512;

  stop_now();

  if ( DEBUG ) {
    Serial.println( "Listo..." );
  }

  // start values, joystick in center positions, value = 512 for both
  Xpos = oldXpos = Xdef;
  Ypos = oldYpos = Ydef;
  
}

void loop() {

looping:

  // reset chars array
  receivedChars[0] = '\0';
  // reset button value
  // nButton = 0;
  // reset string
  bt_read_result = "";

  // received values with finger is on the slide of the joystick app emulator
  if ( BT_master.available() > 0 ) {
    // Data incoming!
    while ( BT_master.available() > 0 ) {
      // read 1 char from input
      char recieved = BT_master.read();
      if ( recieved != '\n' ) {
        bt_read_result += recieved;
      }
    }

    if (bt_read_result.substring(0, 1) == "B") {
      bt_read_result.substring(1, 2).toCharArray(receivedChars, 2);
      nButton = atol( receivedChars);
      if ( blueDEBUG ) {
        Serial.print("Button pressed: ");
        Serial.print(nButton);
      }

      if ( moveDEBUG && nButton == 1 ) {
        Serial.print(" - Emergency stop pressed");
      }

      if ( moveDEBUG && nButton == 2 ) {
        Serial.print(" - Movement locked");
      }

      if ( moveDEBUG && nButton == 3 ) {
        Serial.print(" - Movement restarted");
      }
    }

    if (bt_read_result.substring(0, 1) == "S") {
      bt_read_result.substring(1, 5).toCharArray(receivedChars, 5);
      Xpos = atol( receivedChars);
      if ( blueDEBUG ) {
        Serial.print(" - X position: ");
        Serial.print(Xpos);
      }
    }

    if (bt_read_result.substring(6, 7) == "D") {
      bt_read_result.substring(7, 11).toCharArray(receivedChars, 5);
      Ypos = atol( receivedChars);
      if ( blueDEBUG ) {
        Serial.print(" - Y position: ");
        Serial.print(Ypos);
      }
    }

    if ( blueDEBUG ) { Serial.println(); }
  }

  switch ( nButton ) {
    case 1:
      // emergency stop
      stop_now();
      nButton = 0;
    break;

    case 2:
      // locked
      goto looping;
    break;

    case 3:
      nButton = 0;
      goto looping;
    break;
  }

  /*
   *    forward/backward cases
  */

  if ( Xpos >= ( Xdef + tolerance ) || Xpos <= ( Xdef - tolerance ) ) {
    // actual joystick position not equal last position move not affected, speed not affected
    if ( Xpos != oldXpos ) {
      if ( Xpos > Xdef ) {
        // go forward
/*        
        if ( moveDEBUG ) {
          Serial.print("Fordward - marcha: ");
          Serial.print( Xpos );
          Serial.print(" valor anterior: ");
          Serial.println( oldXpos );
        }
*/
        // test old direction, backward case
        if ( oldXpos <= Xdef ) {
          if ( moveDEBUG ) { Serial.println("Stop and forward"); }
          stop_now();
          if ( oldDirection != "f" ) {
            set_motors( "f" );
          }
        }

        if ( oldXpos > Xdef && Xpos > oldXpos ) {
          if ( oldDirection != "f" ) {
            set_motors( "f" );
          }
        }
        
        velocity = map( Xpos, Xdef, MaxScale, 0, MaxSpeed );
        if ( velocity != oldVelocity ) {
          set_speed( velocity );
        }
      } else if ( Xpos < Xdef ) {
        // go backward
/*
        if ( moveDEBUG ) {
          Serial.print("Backward - marcha: ");
          Serial.print( Xpos );
          Serial.print(" valor anterior: ");
          Serial.println( oldXpos );
        }
*/
        // test old direction, forward case
        if ( oldXpos >= Xdef ) {
          if ( moveDEBUG ) { Serial.println("Stop and backward"); }
          stop_now();
          if ( oldDirection != "b" ) {
            set_motors( "b" );
          }
        }

        if ( oldXpos < Xdef && Xpos < oldXpos ) {
          if ( oldDirection != "b" ) {
            set_motors( "b" );
          }
        }
        
        velocity = map( Xpos, Xdef, 0, 0, MaxSpeed );
        set_speed( velocity );
      } else if ( Xpos == Xdef && Ypos == Ydef ) {
        // all values are in the default
        if ( moveDEBUG ) {
          Serial.println("Stop motors");
        }
  
        stop_now();
      }
    }
  } else {
    // all joystick released, center position
    if ( Xpos == Xdef && Ypos == Ydef and velocity != 0 ) {
      velocity = map( Xpos, Xdef, MaxScale, 0, MaxSpeed );
      if ( velocity != oldVelocity ) {
        set_speed( velocity );
        stop_now();
      }      
    }
  }

  /*
   * right/left cases vehicle stopped or moving
   *
  */

  if ( Ypos != oldYpos ) {
    // stop all motors
    if ( Xpos == Xdef && Ypos == Ydef ) {
      if ( moveDEBUG ) {
        Serial.println("Stop turning");
      }
      stop_now();
    }
  
    // case: vehicle stopped turn on right
    if ( Xpos == Xdef && Ypos > Ydef ) {
      if ( oldDirection != "l" ) {
        stop_now();
        set_motors( "l" );
      }
        
      turn_angle = map( Ypos, Ydef, MaxScale, 0, MaxSpeed );
      set_speed( turn_angle );
    
      if ( moveDEBUG ) {
        Serial.print("Turn right with stopped vehicle: ");
        Serial.println( turn_angle );      
      }        
    }
    
    // case: vehicle stopped turn on left
    if ( Xpos == Xdef && Ypos < Ydef ) {
      if ( oldDirection != "r" ) {
        stop_now();
        set_motors( "r" );
      }
                
      turn_angle = map( Ypos, Ydef, 0, 0, MaxSpeed );
      set_speed( turn_angle );
  
      if ( moveDEBUG ) {
        Serial.print("Turn left with stopped vehicle: ");
        Serial.println( turn_angle );
      }
    }

    // reset original velocity for all motors forward or backward
    if ( Xpos != Xdef && Ypos == Ydef && velocity != 0) {
      motor1.setSpeed( velocity );
      motor2.setSpeed( velocity );
      motor3.setSpeed( velocity );
      motor4.setSpeed( velocity );      
    }

    // turn on right moving forward, decrease right motors speed
    if ( Xpos > Xdef and Ypos > Ydef ) {
      // depending velocity and angle decrease velocity of right motors
      int vdecrease = map( Ypos, Xdef, MaxScale, 0, velocity );
      
      motor1.setSpeed( velocity - vdecrease );
      motor2.setSpeed( velocity - vdecrease );      

      if ( moveDEBUG ) {  
        Serial.print("Turn right with forward advance vehicle: ");
        Serial.println( velocity - vdecrease );
      }
    }

    // turn on left moving forward, decrease left motors speed
    if ( Xpos > Xdef and Ypos < Ydef ) {
      // depending velocity and angle decrease velocity of left motors
      int vdecrease = map( Ypos, Xdef, 0, 0, velocity );
      
      motor3.setSpeed( velocity - vdecrease );
      motor4.setSpeed( velocity - vdecrease );            

      if ( moveDEBUG ) {
        Serial.print("Turn left with forward advance vehicle: ");
        Serial.println( velocity - vdecrease );
      }
    }

    // turn on right moving backguard, decrease left motors speed
    if ( Xpos < Xdef and Ypos > Xdef ) {
      // depending velocity and angle decrease velocity of right motors
      int vdecrease = map( Ypos, Xdef, MaxScale, 0, velocity );
      
      motor3.setSpeed( velocity - vdecrease );
      motor4.setSpeed( velocity - vdecrease );            

      if ( moveDEBUG ) {
        Serial.print("Turn right with backward advance vehicle: ");
        Serial.println( velocity - vdecrease );
      }
    }

    // turn on left moving backguard, decrease right motors speed
    if ( Xpos < Xdef and Ypos < Xdef ) {
      // depending velocity and angle decrease velocity of right motors
      int vdecrease = map( Ypos, Xdef, 0, 0, velocity );
      
      motor1.setSpeed( velocity - vdecrease );
      motor2.setSpeed( velocity - vdecrease );      

      if ( moveDEBUG ) {
        Serial.print("Turn left with backward advance vehicle: ");
        Serial.println( velocity - vdecrease );      
      }
    }
    
  }

  // show current velocity for control
  if ( Xpos != oldXpos ) {
    if ( moveDEBUG ) {
      Serial.print("Motors speed: ");
      Serial.println( velocity );
    }
  }

  // replace the old values with the new values
  oldXpos = Xpos;
  oldYpos = Ypos;
  oldVelocity = velocity;

}
