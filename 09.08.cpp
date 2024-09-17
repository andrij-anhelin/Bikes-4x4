#include <Arduino.h>
#include "crsf.h"
  
  // CRSF/ELRS SETTINGS

    #define CRSF_MIN_VAL 1000
    #define CRSF_MID_VAL 1500
    #define CRSF_MAX_VAL 2000
    #define SBUS_BUFFER_SIZE 32

    uint8_t _rcs_buf[SBUS_BUFFER_SIZE] {};
    uint16_t _raw_rc_count{};
    uint16_t _raw_rc_values[RC_INPUT_MAX_CHANNELS] {};
    
    #define MOVE_CHANNEL  0 // RIGHT_JOYSTICK_Y
    #define TURN_CHANNEL  1 //  LEFT_JOYSTICK_X
    #define BTN_A_CHANNEL 6
    #define BTN_B_CHANNEL 7
    #define BTN_C_CHANNEL 8
    #define BTN_D_CHANNEL 9

    #define CRSF_RX_PIN 17
    #define CRSF_TX_PIN 16
  
    #define COEF_FULL_SPEED 75
    #define COEF_LOW_SPEED 55
    
    #define COEF_TURN_FAST 1.35
    #define COEF_TURN_SLOW 0.75

    #define COEF_BALANCE_R 1.15

  // end CRSF/ELRS SETTINGS

  // I/O SETTINGS

    #define DAC_PIN_R 25
    #define DAC_PIN_L 26

    #define BRAKE_PIN 21

    #define REVERSE_R_PIN 22
    #define REVERSE_L_PIN 23

    int throttle = 0;
    int reverseButton = 0;
    int speed_coef = COEF_LOW_SPEED; 
  
    bool connected = false;
    bool back_R = false;
    bool back_L = false;
    bool _brake = false;

    String _move = "FREEZE";
    String _turn = "-";

    #define ON true
    #define OFF false
  // end I/O SETTING


/*****************************************************************************/

void clearBuffer(){
    for (auto &buf : _rcs_buf)       { buf=0; }
    for (auto &raw : _raw_rc_values) { raw=CRSF_MID_VAL; }
    
    //for (int i = 0; i < SBUS_BUFFER_SIZE; i++)      { _rcs_buf[i] = 0; }
    //for (int i = 0; i < RC_INPUT_MAX_CHANNELS; i++) { _raw_rc_values[i] = CRSF_MID_VAL; }
    
}

void writeFrequency(int pin, int val){
    
  /*
      int lowerEdge; int higherEdge;

      if (val >= 0) {
      lowerEdge = 0;
      higherEdge = CRSF_MID_VAL - CRSF_MIN_VAL;
      } else {
      lowerEdge = CRSF_MIN_VAL - CRSF_MID_VAL; 
      higherEdge = 0;
      }

      int duty = map(val, lowerEdge, higherEdge, 0, 255);
      dacWrite(pin, duty);
      //val = connected ? val: CRSF_MID_VAL;
  */

    int duty = map(val, 0, (CRSF_MID_VAL-CRSF_MIN_VAL), 0, 255);
    dacWrite(pin, duty);
    
  /*
      Serial.printf("\nwriteFrequency: %i", pin);
      Serial.printf("\t > %S", _move);
      Serial.printf("\t > %i", val);
      Serial.printf("\t > %i", duty);
  */
  
}

void setBrake(bool _brake_ON ){
  if(_brake_ON){
    clearBuffer(); 
    writeFrequency(DAC_PIN_R, 0);
    writeFrequency(DAC_PIN_L, 0);
    pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);
    _brake = ON;
  }else{
    pinMode(BRAKE_PIN, PULLUP);
    //pinMode(REVERSE_R_PIN, PULLUP);
    //pinMode(REVERSE_L_PIN, PULLUP);
    _brake = OFF;
  }
}

String getDirection(){

  bool _brake   = _raw_rc_values[BTN_A_CHANNEL]>CRSF_MIN_VAL && _raw_rc_values[BTN_A_CHANNEL]!=CRSF_MID_VAL;
  int throttle = _raw_rc_values[MOVE_CHANNEL] - CRSF_MID_VAL;
  //Serial.printf("\nthrottle: %i", throttle);

  if(_brake){
    return "BRAKE";
  } else if( throttle > 25 ){
    return "FORWARD";
  } else if( throttle <-25 ){
    return "BACK";
  } else {
    return "FREEZE";
  }

}



String getTurn(){

  int turn = _raw_rc_values[TURN_CHANNEL] - CRSF_MID_VAL;
  
  if(turn>15){
    return "R";
  }else if(turn<-15){
    return "L";
  }else{
    return "-";
  }

}

void noMove(){

  clearBuffer(); 
   
  //Serial.print("\tBRAKE ON");
  /*@ pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);    _brake=true; @*/

  writeFrequency(DAC_PIN_R, 0);
  writeFrequency(DAC_PIN_L, 0);

  //setBrake(ON);

}





/******************************************************************************/

void setup() {

  Serial.begin(9600);//38400
  Serial2.begin(420000, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);
  
  Serial.print("\n\n###############################\n##       READY TO RIDE       ##\n###############################\n\n\n\n");

  //connected = Serial2.readBytes(_rcs_buf, SBUS_BUFFER_SIZE)>0;
  setBrake(ON);
  pinMode(REVERSE_R_PIN, PULLUP);
  pinMode(REVERSE_L_PIN, PULLUP);
  noMove();

}

/******************************************************************************/


void loop() { 
  //while (true) {
  if (!Serial2.available()) {
      //Serial.print("\n LOST CONNECTION");
      //pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);
      noMove();
  } else {
    size_t numBytesRead = Serial2.readBytes(_rcs_buf, SBUS_BUFFER_SIZE);
    connected = numBytesRead > 0;
    if(!connected) {
        //pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);
        noMove();
    } else {
        
        crsf_parse(&_rcs_buf[0], SBUS_BUFFER_SIZE, &_raw_rc_values[0], &_raw_rc_count, RC_INPUT_MAX_CHANNELS );

        /*
          int btnA=_raw_rc_values[BTN_A_CHANNEL]-CRSF_MID_VAL;
          int btnB=_raw_rc_values[BTN_B_CHANNEL]-CRSF_MID_VAL;
          int btnC=_raw_rc_values[BTN_C_CHANNEL]-CRSF_MID_VAL;
          int btnD=_raw_rc_values[BTN_D_CHANNEL]-CRSF_MID_VAL;
          
          Serial.printf( "\n A:%i\t B:%i\t C:%i\t D:%i", btnA, btnB, btnC, btnD );
        */
        
        //Serial.println(_raw_rc_values[BTN_C_CHANNEL]);

        if(_raw_rc_values[BTN_C_CHANNEL]>CRSF_MID_VAL){
          if(speed_coef!=COEF_LOW_SPEED){ 
            //Serial.printf( "\nLOW SPEED\t%i > %i", speed_coef, COEF_LOW_SPEED ); 
            speed_coef=COEF_LOW_SPEED;
          }
        } else if(_raw_rc_values[BTN_C_CHANNEL]<CRSF_MID_VAL) {
          if(speed_coef!=COEF_FULL_SPEED){ 
            //Serial.printf("\nFULL SPEED\t%i > %i",speed_coef, COEF_FULL_SPEED); 
            speed_coef=COEF_FULL_SPEED;
          }
        } else {
          //speed_coef=0;
        }
        
        _move = getDirection();
        _turn = getTurn();

        int speed_R = 0;
        int speed_L = 0;
        int val_speed = abs(_raw_rc_values[MOVE_CHANNEL] - CRSF_MID_VAL) * speed_coef * .01;
        int val_turn  = abs(_raw_rc_values[TURN_CHANNEL] - CRSF_MID_VAL) * speed_coef * .01 * .85;

        bool turn_mode = _raw_rc_values[BTN_B_CHANNEL]<CRSF_MID_VAL;
  
        _brake=false;
        //Serial.printf("\n move: %s \t turn: %s \t%i", _move, _turn, val_turn);

      if (_move=="BRAKE") { // [A]>MIN && [A]!=MID

			  /* BRAKE BUTTON PRESSED */
			  /*
          writeFrequency(DAC_PIN_R, 0);
          writeFrequency(DAC_PIN_L, 0);
          pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);    
          _brake=true;
        */
        setBrake(ON);
        Serial.printf("\nBRAKE ON\t%i",_raw_rc_values[BTN_A_CHANNEL]);

      } else if (_move=="FORWARD") {
          
        setBrake(OFF);
        pinMode(REVERSE_R_PIN, PULLUP);      back_R=false;
        pinMode(REVERSE_L_PIN, PULLUP);      back_L=false;

        if(_turn=="R"){
          speed_L = val_speed * (turn_mode? 1: COEF_TURN_FAST);
          speed_R = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
        }else if(_turn=="L"){
          speed_L = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
          speed_R = val_speed * (turn_mode? 1: COEF_TURN_FAST);
        }else{
          speed_L = speed_R = val_speed;
        }
          
        //Serial.printf("\n turn mode: %s \t%i\t%i", (turn_mode? "-|-":"||"), speed_L, speed_R);

        writeFrequency(DAC_PIN_R, speed_R * COEF_BALANCE_R);
        writeFrequency(DAC_PIN_L, speed_L);
        val_turn = speed_L - speed_R;

      } else if (_move=="BACK"){

        setBrake(OFF);
        pinMode(REVERSE_L_PIN, OUTPUT_OPEN_DRAIN);  back_L=true;
        pinMode(REVERSE_R_PIN, OUTPUT_OPEN_DRAIN);  back_R=true;

        if(_turn=="R"){
            speed_L = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
            speed_R = val_speed * (turn_mode? 1: COEF_TURN_FAST);
        } else if(_turn=="L"){
            speed_L = val_speed * (turn_mode? 1: COEF_TURN_FAST);
            speed_R = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
        }else{
            speed_L = speed_R = val_speed;
        }
          
        writeFrequency(DAC_PIN_R, speed_R * COEF_BALANCE_R);
        writeFrequency(DAC_PIN_L, speed_L);
        val_turn = speed_L - speed_R;

      } else if (_move=="FREEZE"){ 
          
          setBrake(OFF);

          if(_turn=="R"){
            
            pinMode(REVERSE_R_PIN, OUTPUT_OPEN_DRAIN);  back_R=true; //digitalWrite(REVERSE_R_PIN, HIGH);
            pinMode(REVERSE_L_PIN, PULLUP);             back_L=false;
            
            speed_R = val_turn * 0.9;
            speed_L = val_turn * 1.2;

            writeFrequency(DAC_PIN_R, speed_R * COEF_BALANCE_R);
            writeFrequency(DAC_PIN_L, speed_L);

          } else if(_turn=="L"){

            pinMode(REVERSE_R_PIN, PULLUP);             back_R=false; //digitalWrite(REVERSE_R_PIN, HIGH); 
            pinMode(REVERSE_L_PIN, OUTPUT_OPEN_DRAIN);  back_L=true;  //digitalWrite(REVERSE_L_PIN, HIGH); 
            
            speed_R = val_turn * 0.9;
            speed_L = val_turn * 1.2;

            writeFrequency(DAC_PIN_R, speed_R * COEF_BALANCE_R);
            writeFrequency(DAC_PIN_L, speed_L);

          } else {
            //setBrake(ON);
            noMove();
            /*
            pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);    _brake=true;
            pinMode(REVERSE_R_PIN, PULLUP);           back_R=false;
            writeFrequency(DAC_PIN_R, 0);
            writeFrequency(DAC_PIN_L, 0);
            */   
          }
          
                   
      } else { // no more moves
          //setBrake(ON);
          noMove();
      }

        //if(val_turn){
        /*  Serial.printf(
            "\n %s \t %s: %s==%s \t %i \t %s", 
              _move, 
              _turn, 
              back_L?"▼":"▲", 
              back_R?"▼":"▲", 
              val_turn,
              _brake?"BRAKE ON":""
          );
        //}*/
      }
  }
  //}
}
