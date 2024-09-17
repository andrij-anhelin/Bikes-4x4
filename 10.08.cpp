#include <Arduino.h>
#include "crsf.h"
  
  // CRSF/ELRS SETTINGS

    #define CRSF_MIN_VAL 1000
    #define CRSF_MID_VAL 1500
    #define CRSF_MAX_VAL 2000
    #define SBUS_BUFFER_SIZE 32
    
    #define MOVE_CHANNEL  0 // RIGHT_JOYSTICK_Y
    #define TURN_CHANNEL  1 //  LEFT_JOYSTICK_X
    #define BTN_A_CHANNEL 6
    #define BTN_B_CHANNEL 7
    #define BTN_C_CHANNEL 8
    #define BTN_D_CHANNEL 9

    #define CRSF_RX_PIN 17
    #define CRSF_TX_PIN 16
  
    #define COEF_FULL_SPEED 75
    #define COEF_LOW_SPEED  55
    
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

    #define ON true
    #define OFF false

    #define NONE    0
    #define FORWARD 1
    #define BACK   -1
    #define BRAKE  -9

    #define LEFT   -5  
    #define RIGHT   5
        
    uint8_t _rcs_buf[SBUS_BUFFER_SIZE] {};
    uint16_t _raw_rc_count{};
    uint16_t _raw_rc_values[RC_INPUT_MAX_CHANNELS] {};

    bool connected = false;
    int  _move = NONE;
    int  _turn = NONE;
    bool _brake = false;
    bool back_L = false;
    bool back_R = false;
    
    int throttle = 0;
    int speed_coef = COEF_LOW_SPEED;
    int speed_L = 0;
    int speed_R = 0;
        
  // end I/O SETTING


/*****************************************************************************/

void clearBuffer(){
    for (auto &buf : _rcs_buf)       { buf=0; }
    for (auto &raw : _raw_rc_values) { raw=CRSF_MID_VAL; }
    
    //for (int i = 0; i < SBUS_BUFFER_SIZE; i++)      { _rcs_buf[i] = 0; }
    //for (int i = 0; i < RC_INPUT_MAX_CHANNELS; i++) { _raw_rc_values[i] = CRSF_MID_VAL; }
    
}

void writeFrequency(int pin, int val){
    int duty = map(val, 0, (CRSF_MID_VAL-CRSF_MIN_VAL), 0, 255);
    dacWrite(pin, duty);
}

void setSpin(int _Left=0, int _Right=0){
  if(!_Left&&!_Right){
    clearBuffer();
  }
  writeFrequency(DAC_PIN_R, _Right * COEF_BALANCE_R);
  writeFrequency(DAC_PIN_L, _Left);
}

void setBrake(bool brakeON ){
  if(brakeON){
    clearBuffer(); 
    setSpin(NONE);
    pinMode(BRAKE_PIN, OUTPUT_OPEN_DRAIN);
    _brake = ON;
  }else{
    pinMode(BRAKE_PIN, PULLUP);
    _brake = OFF;
  }
}

void setPins(int dir){
  switch(dir){
    case FORWARD:
      pinMode(REVERSE_R_PIN, PULLUP);             back_R=false;
      pinMode(REVERSE_L_PIN, PULLUP);             back_L=false;
      return;
    case BACK: 
      pinMode(REVERSE_R_PIN, OUTPUT_OPEN_DRAIN);  back_R=false;
      pinMode(REVERSE_L_PIN, OUTPUT_OPEN_DRAIN);  back_L=false;
      return;
    case RIGHT: 
      pinMode(REVERSE_R_PIN, OUTPUT_OPEN_DRAIN);  back_R=true; 
      pinMode(REVERSE_L_PIN, PULLUP);             back_L=false;
      return;      
    case LEFT:
      pinMode(REVERSE_R_PIN, PULLUP);             back_R=false; 
      pinMode(REVERSE_L_PIN, OUTPUT_OPEN_DRAIN);  back_L=true;
      return;
    default:
      // do nothing ?
      return;
  }
}


int getDirection(){

  bool brakeON = _raw_rc_values[BTN_A_CHANNEL]>CRSF_MIN_VAL && _raw_rc_values[BTN_A_CHANNEL]!=CRSF_MID_VAL;
  int throttle = _raw_rc_values[MOVE_CHANNEL] - CRSF_MID_VAL;

  if(brakeON){              return BRAKE; } 
  else if( throttle > 25 ){ return FORWARD; } 
  else if( throttle <-25 ){ return BACK; } 
  else {                    return NONE; }

}

int getTurn(){

  int turn = _raw_rc_values[TURN_CHANNEL] - CRSF_MID_VAL;
  
  if(turn>15){
    return RIGHT;
  }else if(turn<-15){
    return LEFT;
  }else{
    return NONE;
  }

}

int getSpeedCoef(){
  if(_raw_rc_values[BTN_C_CHANNEL]>CRSF_MID_VAL){
    /*if(speed_coef!=COEF_LOW_SPEED){ 
      Serial.printf( "\nLOW SPEED\t%i > %i", speed_coef, COEF_LOW_SPEED ); 
    }*/
    return COEF_LOW_SPEED;
  } else if(_raw_rc_values[BTN_C_CHANNEL]<CRSF_MID_VAL) {
    /*if(speed_coef!=COEF_FULL_SPEED){ 
      Serial.printf("\nFULL SPEED\t%i > %i", speed_coef, COEF_FULL_SPEED); 
    }*/
    return COEF_FULL_SPEED;
    
  } /*else {
    return speed_coef;
  }*/
  return speed_coef;
}

void noMove(){

  clearBuffer(); 
  setSpin(NONE);
  //setBrake(ON);

}

/******************************************************************************/

void setup() {
  Serial.begin(9600);//38400
  Serial2.begin(420000, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);
  Serial.print("\n\n###############################\n##       READY TO RIDE       ##\n###############################\n\n\n\n");
  //connected = Serial2.readBytes(_rcs_buf, SBUS_BUFFER_SIZE)>0;
  setPins(FORWARD);
  noMove();
  setBrake(ON);
  
}

/******************************************************************************/

void loop() { 
  while(Serial2.available()){

    size_t numBytesRead = Serial2.readBytes(_rcs_buf, SBUS_BUFFER_SIZE);
    connected = numBytesRead > 0;

    if(connected){
        
      crsf_parse(&_rcs_buf[0], SBUS_BUFFER_SIZE, &_raw_rc_values[0], &_raw_rc_count, RC_INPUT_MAX_CHANNELS);
       
      _move = getDirection();
      _turn = getTurn();
      speed_coef = getSpeedCoef();

      int val_speed  = abs(_raw_rc_values[MOVE_CHANNEL] - CRSF_MID_VAL) * speed_coef * .01;
      int turn_speed = abs(_raw_rc_values[TURN_CHANNEL] - CRSF_MID_VAL) * speed_coef * .01 * .85;

      bool turn_mode = _raw_rc_values[BTN_B_CHANNEL] < CRSF_MID_VAL;
  
      //Serial.printf("\n move: %s \t turn: %s \t%i", _move, _turn, turn_speed);
      //setSpin(NONE);
      setSpin(speed_L,speed_R);

      switch(_move){

        case FORWARD:

          setBrake(OFF);
          setPins(FORWARD);
          
          switch(_turn){
            case RIGHT:
              speed_L = val_speed * (turn_mode? 1: COEF_TURN_FAST);
              speed_R = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
              break;
            case LEFT:
              speed_L = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
              speed_R = val_speed * (turn_mode? 1: COEF_TURN_FAST);
              break;
            default:
              speed_L = speed_R = val_speed;
          }

          turn_speed = speed_L - speed_R;       
          setSpin(speed_L,speed_R);
          break;

        case BACK:

          setBrake(OFF);
          setPins(BACK);
          
          val_speed=val_speed*1.2;

          switch(_turn){
            case RIGHT:
              speed_L = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
              speed_R = val_speed * (turn_mode? 1: COEF_TURN_FAST);
              break;
            case LEFT:
              speed_L = val_speed * (turn_mode? 1: COEF_TURN_FAST);
              speed_R = val_speed * (turn_mode? 0: COEF_TURN_SLOW);
              break;
            default:
              speed_L = speed_R = val_speed;
          }

          turn_speed = speed_L - speed_R;
          setSpin(speed_L,speed_R);
          break;
        
        case BRAKE: // [A]>MIN && [A]!=MID

          /* BRAKE BUTTON PRESSED */
          setBrake(ON);
          Serial.printf("\nBRAKE ON\t%i",_raw_rc_values[BTN_A_CHANNEL]);
          break;

        case NONE:

          setBrake(OFF);
          setPins(_turn);

          switch(_turn){
            case RIGHT:
              speed_L = turn_speed * 1.2;
              speed_R = turn_speed * 0.9; // back
              break;
            case LEFT:
              speed_L = turn_speed * 1.2; // back
              speed_R = turn_speed * 0.9;
              break;
            default:
              speed_L = speed_R = 0;
          }
          setSpin(speed_L,speed_R);
          break;

        default:
          //setBrake(ON);
          noMove();
          break;
      }

      /* 
        Serial.printf(
            "\n %s \t %s: %s==%s \t %i \t %s", 
            _move, 
            _turn, 
            back_L?"▼":"▲", 
            back_R?"▼":"▲", 
            turn_speed,
            _brake?"BRAKE ON":""
          );
      */

    }else{
      //Serial.println("ERROR: LOST CONNECTION !!!");
      noMove();
      setBrake(ON);
    }
  }
  //Serial.println("ERROR: SERIAL2 DISABLED !!!");
  noMove();
  setBrake(ON);
 
}
