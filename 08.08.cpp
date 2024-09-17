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
  

  #define TURN_CHANNEL 1
  #define MOVE_CHANNEL 0
  #define BTN_A_CHANNEL 6
  #define BTN_B_CHANNEL 7
  #define BTN_C_CHANNEL 8
  #define BTN_D_CHANNEL 9

  #define CRSF_RX_PIN 17
  #define CRSF_TX_PIN 16
 
  #define COEF_FULL_SPEED 75
  #define COEF_LOW_SPEED 60
  
  #define COEF_TURN_FAST 1.25
  #define COEF_TURN_SLOW 0.85
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
// end I/O SETTINGS

/******************************************************************************/

void writeFrequency(int pin, int val) {
  
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

String getDirection(){

  int throttle = _raw_rc_values[MOVE_CHANNEL]- CRSF_MID_VAL;
  //Serial.printf("\nthrottle: %i", throttle);

  if( throttle > 25 ){
    return "FORWARD";
  } else 
  if( throttle < -25 ){
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

  // Clean buffer 

    //for (auto &buf : _rcs_buf)       { buf=0; }
    //for (auto &raw : _raw_rc_values) { raw=CRSF_MID_VAL; }
    
    for (int i = 0; i < SBUS_BUFFER_SIZE; i++)      { _rcs_buf[i] = 0; }
    for (int i = 0; i < RC_INPUT_MAX_CHANNELS; i++) { _raw_rc_values[i] = CRSF_MID_VAL; }
    
  //Serial.print("\tBRAKE ON");
  /*@ pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);    _brake=true; @*/

  writeFrequency(DAC_PIN_R, 0);
  writeFrequency(DAC_PIN_L, 0);

}

/******************************************************************************/


void setup() {

  Serial.begin(9600);
  Serial2.begin(420000, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);
  
  Serial.print("\n\n###############################\n##       READY TO RIDE       ##\n###############################\n\n\n\n");

  //connected = Serial2.readBytes(_rcs_buf, SBUS_BUFFER_SIZE)>0;
  pinMode(BRAKE_PIN, PULLUP);
  pinMode(REVERSE_R_PIN, PULLUP);
  pinMode(REVERSE_L_PIN, PULLUP);
  noMove();

}

/******************************************************************************/


void loop() { 
  //while (true) {
  if ( !Serial2.available()) {
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
        } else 
        if(_raw_rc_values[BTN_C_CHANNEL]<CRSF_MID_VAL) {
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
        int val_speed = abs(_raw_rc_values[MOVE_CHANNEL]- CRSF_MID_VAL) * speed_coef * .01;
        int val_turn  = abs(_raw_rc_values[TURN_CHANNEL] - CRSF_MID_VAL) * speed_coef * .01;

        bool turn_mode = _raw_rc_values[BTN_B_CHANNEL]<CRSF_MID_VAL;
  
        _brake=false;
        //Serial.printf("\n move: %s", _move);
        //Serial.printf("\t turn: %s", _turn); 
        //Serial.printf("\t%i\n", val_turn);

        if(_raw_rc_values[BTN_A_CHANNEL]<CRSF_MID_VAL) {

			/* BRAKE BUTTON PRESSED */
			writeFrequency(DAC_PIN_R, 0);
			writeFrequency(DAC_PIN_L, 0);
			pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);    
			_brake=true;

		} else 
		if (_move=="FORWARD") {
          
          pinMode(BRAKE_PIN, PULLUP);
          
          //if(back_R||back_L){
            back_R=false;
            back_L=false;
            pinMode(REVERSE_R_PIN, PULLUP);
            pinMode(REVERSE_L_PIN, PULLUP);
          //} 
          

          if(_turn=="R"){
            speed_L = val_speed * (turn_mode? 1: COEF_TURN_FAST);
            speed_R = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
          } else if(_turn=="L"){
            speed_L = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
            speed_R = val_speed * (turn_mode? 1: COEF_TURN_FAST);
          }else{
            speed_L = speed_R = val_speed;
          }
          
          Serial.printf("\nturn mode: %s \t%i\t%i", turn_mode? "-|-":"||", speed_L, speed_R);

          writeFrequency(DAC_PIN_R, speed_R);
          writeFrequency(DAC_PIN_L, speed_L);
          val_turn = speed_L - speed_R;

        } else 
        if (_move=="BACK"){

          pinMode(BRAKE_PIN, PULLUP);
          
          //if(!back_R||!back_L){
            back_R=true;
            back_L=true;
            pinMode(REVERSE_R_PIN, OUTPUT_OPEN_DRAIN);
            pinMode(REVERSE_L_PIN, OUTPUT_OPEN_DRAIN);
          //}

          if(_turn=="R"){
            speed_L = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
            speed_R = val_speed * (turn_mode? 1: COEF_TURN_FAST);
          } else if(_turn=="L"){
            speed_L = val_speed * (turn_mode? 1: COEF_TURN_FAST);
            speed_R = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
          }else{
            speed_L = speed_R = val_speed;
          }
          
          writeFrequency(DAC_PIN_R, speed_R);
          writeFrequency(DAC_PIN_L, speed_L);
          val_turn = speed_L - speed_R;

        } else 
        if (_move=="FREEZE"){ 
          
          pinMode(REVERSE_R_PIN, PULLUP);             back_R=false;
          pinMode(REVERSE_L_PIN, PULLUP);             back_L=false;
          noMove();
          //Serial.printf("\n turn: %s\t> %i", _turn, val_turn);

          if(_turn=="R"){

            pinMode(BRAKE_PIN, PULLUP);                 _brake=false;

            pinMode(REVERSE_R_PIN, OUTPUT_OPEN_DRAIN);  back_R=true;
            pinMode(REVERSE_L_PIN, PULLUP);             back_L=false;
            
            speed_R = val_turn * 1.1;
            speed_L = val_turn * 0.9;

            writeFrequency(DAC_PIN_R, speed_R);
            writeFrequency(DAC_PIN_L, speed_L);

          } else if(_turn=="L"){

            pinMode(BRAKE_PIN, PULLUP);                 _brake=false;

            pinMode(REVERSE_R_PIN, PULLUP);             back_R=false;
            pinMode(REVERSE_L_PIN, OUTPUT_OPEN_DRAIN);  back_L=true;
            
            speed_R = val_turn * 0.9;
            speed_L = val_turn * 1.1;

            writeFrequency(DAC_PIN_R, speed_R);
            writeFrequency(DAC_PIN_L, speed_L);

          } else {
            //pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);    _brake=true;
            /*pinMode(REVERSE_R_PIN, PULLUP);             back_R=false;
            pinMode(REVERSE_L_PIN, PULLUP);             back_L=false;*/
            noMove();
          } /**/
          
                   
        } else {
          // no more moves
          // noMove();
        }             

        //pinMode(REVERSE_BUTTON_PIN, reverseButton?OUTPUT_OPEN_DRAIN:PULLUP);

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
