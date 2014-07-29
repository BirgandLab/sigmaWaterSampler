#include <EEPROM.h>
#include "EEPROMAnything.h"
#include "Chronodot.h"
#include "Wire.h"

//INPUTS
const int gray=            A1;
const int purple=          A2;
const int VBat=            A3;
//A4 SDA RTC
//A5 SCL RTC

const int pumpSwitch=      2;
const int zeroPin=         3;
const int nextGapPin=      4;
const int previousGapPin=  5;

//OUTPUTS
const int powerOpto=       A0;

const int purge=            6;    //pump motor empty lines
const int pump=             7;     //pump motor fill lines and bottles
const int cw=               8;       //turn distributor arm clockwise
const int ccw=              9;      //turn distributor arm counter clockwise


const int dangerZone=2000;
int counter=0;
int bottle;
long int averageSeekTime;
 long int seekTime=0;
 long int sought=0;
int incomingByte = 0; 


void setup(){
  Serial.begin(9600);
// OUTPUTS
 pinMode(powerOpto,OUTPUT);
 pinMode(pump,OUTPUT);
 digitalWrite(pump,HIGH);
 pinMode(cw,OUTPUT);
 pinMode(ccw,OUTPUT);
  digitalWrite(cw,HIGH);
 digitalWrite(ccw,HIGH);
 
 
 pinMode(pumpSwitch,INPUT);
 pinMode(gray,INPUT);
 pinMode(purple,INPUT);
 pinMode(nextGapPin,INPUT);
 pinMode(previousGapPin,INPUT);
 
 //testSeekTime();
 EEPROM_readAnything(0, bottle);

 Serial.println(bottle);
 initializeSampler();
 //findZero();
 EEPROM_writeAnything(0,bottle);
}


void loop(){
            long int elapsed=0;
            long int sought=0;
           if(digitalRead(nextGapPin) &&counter==10){
            advance();
            Serial.println(averageSeekTime);
             counter=0;
           //  bottle=read(bottle);
           }
           
            if(digitalRead(zeroPin) &&counter==10){
             findZero();
             counter=0;
           }
           
           if(digitalRead(pumpSwitch) &&counter==10){
             Serial.println("pumpSwitch");
             digitalWrite(pump,LOW);
             while(digitalRead(pumpSwitch)) { 
               delay(20);
             }
             digitalWrite(pump,HIGH);
             counter=0;
           }
           
            if(digitalRead(previousGapPin) &&counter==10){
              if(bottle==2){
                    findZero();
                     }
                if(bottle!=0){
                   reverse();
                    }
            Serial.println(averageSeekTime);
             counter=0;
           }
            
            
            if(digitalRead(nextGapPin)||digitalRead(previousGapPin) ||digitalRead(zeroPin) || digitalRead(pumpSwitch)){
              counter++;
            }
            else {
              counter=0;
              }
              
           if (Serial.available() > 0) {
                // read the incoming byte:
                incomingByte = Serial.parseInt();
                // say what you got:
                Serial.print("I received: ");
                Serial.println(incomingByte, DEC);
                if(incomingByte>=1 and incomingByte<=24){
                goToBottle(incomingByte);
                }
        }
           
}//end void loop

void advance(){
  long int timer=millis();
  long int danger=millis();
  int  success=0;
  int tries=0;
  int bail=0;
  
//IF at the end of the bottles rezero, but don't advance  
  if (bottle>=24){
          bail=1; 
          Serial.println("already at end of bottles");
          reverse();
          reverse();
          findZero();
          //findZero();
        }
  if (bottle<24 && bail==0){
    //lets try 4x to get the rotor to the next bottle

   
 //TAKE 3 TRIES TO GET PAST ANY OBSTACLES  
    while(!success && tries<=3){
      //Serial.print("try ");
      //Serial.println(tries);
                 long int danger=millis();
                   // Serial.println("advance");
                    
     
         //IF on a gap, move off               
                    if(!digitalRead(purple) && !bail){//if already on a gap
                            while(!digitalRead(purple) && !bail){
                              digitalWrite(cw,LOW);
                              if ((millis()-danger)>dangerZone){bail=1; Serial.println("bail!!");}
                              }
                      digitalWrite(cw,HIGH);
                    }//end if digitalRead(gray)==0;
                    
         //IF not on a gap, look for the next one   
                    danger=millis();
                    //ADD SOMETHING HERE ABOUT AVERAGE SEARCH TIME
                    digitalWrite(cw,LOW);
                    while(digitalRead(purple) && !bail){
                          if ((millis()-danger)>dangerZone){
                                    bail=1; 
                                    Serial.println("bail!!!");
                                  }
                          delay(30);
                          }
                      digitalWrite(cw,HIGH);
                      
            //ONCE WE HAVE FOUND THE NEXT GAP --OR BEEN KICKED OUT BY BAIL
            
                      if(!bail && ((millis()-timer)>(averageSeekTime/2))){
                            bottle++; //update bottle number
                            success=1;  //it worked, don't try again
                            }
                       
                          tries++;  //augment counter, we might need to tray again
                          bail=0;    //RESET BAIL FOR ANOTHER TRY
                    }//while try and try again
                        
  }//end if bottle <23
  
//in all cases        
    timer=millis()-timer;                                     //TOTAL ELAPSED TIME OF THIS FUNCTION
    EEPROM_writeAnything(0,bottle);                          //THE BOTTLE NUMBER WE THINK IT IS ON NOW
    Serial.print("bottle #:"); Serial.println(bottle);
    if(tries==1){         //IF WE ONLY WENT THROUGH THE TRY LOOP ONCE
            seekTime+=timer;             //RETURN HOW LONG IT TOOK
            sought++;
            averageSeekTime=floor(seekTime/sought);
            }
    if (tries!=1) {
      Serial.print("tries = ");
      Serial.println(tries);
             // return averageSeekTime;                        //OTHERWISE, RETURN THIS, CUZ IT IS THE BEST ESTIMATE OF GOOD SEEK TIME
            }
  }






void reverse(){
  long int timer=millis();
  long int danger=millis();
  int  success=0;
  int tries=0;
  int bail=0;

 //TAKE 4 TRIES TO GET PAST ANY OBSTACLES  
    while(!success && tries<=2){
      //Serial.print("try ");
     // Serial.println(tries);
                 long int danger=millis();
                  //  Serial.println("reverse");
                    
     
         //IF on a gap, move off               
                    if(!digitalRead(purple) && !bail){//if already on a gap
                            while(!digitalRead(purple) && !bail){
                              digitalWrite(ccw,LOW);
                              if ((millis()-danger)>dangerZone){bail=1; Serial.println("bail!!");}
                              }
                      digitalWrite(ccw,HIGH);
                    }//end if digitalRead(gray)==0;
                    
         //IF not on a gap, look for the next one   
                    danger=millis();
                    //ADD SOMETHING HERE ABOUT AVERAGE SEARCH TIME
                    digitalWrite(ccw,LOW);
                    while(digitalRead(purple) && !bail){
                          if ((millis()-danger)>dangerZone){
                                    bail=1; 
                                    Serial.println("bail!!!");
                                  }
                        //  delay(30);
                          }
                      digitalWrite(ccw,HIGH);
                      
            //ONCE WE HAVE FOUND THE NEXT GAP --OR BEEN KICKED OUT BY BAIL
           
                      if(!bail && ((millis()-timer)>(averageSeekTime/2))){
                            bottle--; //update bottle number
                            success=1;  //it worked, don't try again
                      }
 
                       
                          tries++;  //augment counter, we might need to tray again
                          bail=0;    //RESET BAIL FOR ANOTHER TRY
                    }//while try and try again
                        
  
//in all cases        
    timer=millis()-timer;                                     //TOTAL ELAPSED TIME OF THIS FUNCTION
    EEPROM_writeAnything(0,bottle);                          //THE BOTTLE NUMBER WE THINK IT IS ON NOW
    Serial.print("bottle #:"); Serial.println(bottle);
    if(tries==1){         //IF WE ONLY WENT THROUGH THE TRY LOOP ONCE
            seekTime+=timer;             //RETURN HOW LONG IT TOOK
            sought++;
            averageSeekTime=floor(seekTime/sought);
            }
    if (tries>1) {
      Serial.print("tries = ");
      Serial.println(tries);
             // return averageSeekTime;                        //OTHERWISE, RETURN THIS, CUZ IT IS THE BEST ESTIMATE OF GOOD SEEK TIME
            }
  }







void reverseGray(){
  long int timer=millis(); //start a timer to keep track of seek time
  long int danger=millis();
  int bail=0; 
            if(digitalRead(gray)==0){//if already on a gap
                digitalWrite(ccw,LOW);
                while(!digitalRead(gray)&&!bail){
                        if((millis()-danger)>dangerZone){
                              bail=1; 
                              Serial.println("bail!!!");
                            }
                        }//END WHILE (!digitalRead(gray)&&!bail)
                digitalWrite(ccw,HIGH);
                }//end if digitalRead(gray)==0;
                
          danger=millis();
          
//NOW SEARCH FOR NEXT GAP
           while(digitalRead(gray) && !bail){
                      digitalWrite(ccw,LOW);
                     if ((millis()-danger)>dangerZone){
                             bail=1;
                             Serial.println("bail!!!");
                           }
           // delay(30);//WAIT A BIT TO GET IN THE MIDDLE OF THE GAP
    }
    digitalWrite(ccw,HIGH);
   // Serial.println("gotIT");
   // if(!bail && color==purple){
   //     bottle--;
   // }
   // }
    Serial.print("bottle #:"); Serial.println(bottle);
    EEPROM_writeAnything(0,bottle);
    timer=millis()-timer; 
  }
  
  
void findZero(){
  long int danger=millis();
  int bail=0;
    if(bottle==24){ //seems to find zero up there
      reverse();
      reverse();
      
    }
    if(bottle<24 && bottle>=10){
      reverse();
    }
    if (bottle<10){ //make sure it is far enough away from the 0 and get a seekTime measurement
         advance();
      moveForward(purple);
      moveForward(gray);
      moveForward(purple);
      moveForward(gray);
        }
    Serial.println("findZero");
    
    digitalWrite(ccw,LOW);
    
    //Wow thi is sorta dangerous
    while(digitalRead(gray) || digitalRead(purple)){
             delay(20);
            //  reverse(); 
            //STOPS AT BOTTLE #2
          }
   digitalWrite(ccw,HIGH);
   
    moveBack(gray);
    moveBack(purple);
    moveBack(gray);
    moveBack(purple);    
    moveBack(gray);
  
          
         // reverseGray();
         // reverseGray();
        //  advance();
          
    Serial.println("gotIT");
    bottle=1;
    Serial.print("bottle #:"); Serial.println(bottle);
    EEPROM_writeAnything(0,bottle);
  }
  
  
  void initializeSampler(){
          //long int seekTime=0;
          Serial.println("initializing sampler");
          if(bottle<20){
          advance();
          advance();
          averageSeekTime=floor(seekTime/sought);
          Serial.println(averageSeekTime);
          }
    
    findZero();
     
  }
  
    void moveBack(int color){
      long int danger=500;
      long int watch=millis();
      digitalWrite(ccw,LOW);
     while(!digitalRead(color)&& ((millis()-watch)<danger)){
       delay(20);
     }
     digitalWrite(ccw,HIGH);
  }
  
    
    void moveForward(int color){
      long int danger=500;
      long int watch=millis();
    digitalWrite(cw,LOW);
     while(!digitalRead(color) && ((millis()-watch)<danger)){
       delay(20);
     }
     digitalWrite(cw,HIGH);
  }
  
void goToBottle(int target){
  if(target==1){
     while(target>2){
       reverse();
      }
      
      findZero();
  }
  if(target!=1){
    while(target>bottle){
      long int time=millis();
      digitalWrite(pump,LOW);
      while ((millis()-time)<3000){
        
      }
      digitalWrite(pump,HIGH);
      
      advance();
    }
    
    while(target<bottle){
      reverse();
    }
    
  }
    if(target==bottle){      
      Serial.println("rejoice");
     //  return 0;  
    }
      
    
  }
  
  
  
  void simplerAdvanceFunction( ){
    long int timer=millis();   //identify start time of function
    long int timer2=millis();
    long int minimumTotalSeekTime=floor(averageSeekTime*0.8);
    int phase2Success=0;
    int phase1Success=0;
    long int phase1SeekTime=floor(averageSeekTime/3);
    int phase2=0;
     long int phase2SeekTime=floor(averageSeekTime/3);
    int phase3=0;
     long int phase3SeekTime=floor(averageSeekTime/3);
    int plase4=0;
     long int phase4SeekTime=floor(averageSeekTime/3);
     
            //if we find the next bottle too quickly, reverse, and try again
            //failSave may require not backtracking more than incrementally (less than a bottle)
            //we don't want to get in a position where we corrupt already collected samples
    //turn on opto couplers
    
    
          digitalWrite(cw,HIGH);
          while( digitalRead(purple) && !digitalRead(gray) && ((millis()-timer2)<phase1SeekTime)){
            delay(10);
          }
    digitalWrite(cw,LOW);
    //If we got the anticipated phase change, it is a success
          if(!digitalRead(purple) && !digitalRead(gray)){
           phase1Success=1; 
          }
          
      
      
          
     digitalWrite(cw,HIGH);
          timer2=millis();
              while(!digitalRead(purple) && !digitalRead(gray) && ((millis()-timer2)<phase2SeekTime)){
                delay(10);
              }
      digitalWrite(cw,LOW);
    //If we got the anticipated phase change, it is a success
          if(!digitalRead(purple) && digitalRead(gray)){
           phase2Success=1; 
          }

    //the cycle would go from Purple Gray
    //                          1     0 //front of gap this is where we usually stop
    //                          0     0
    //                          0     1//front of gap on second sensor
    //                          0     0
    //                          1     0
    
    
    //possible moves: gray-move off gap 
    //                gray-move to next gap
    //                purple-move off gap
    //                purple-move to next gap
    // accomplish whole move in some time similar to average search time
    //accomplish portions of the move in some time similar to their proportion
    //Make several attempts to move past an obstacle
    //make sure that enough time has passed so that false positives are unlikely
       
       //turn off opto couplers confident that the move has been made
  }
  

