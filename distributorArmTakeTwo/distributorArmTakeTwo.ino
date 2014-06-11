#include <EEPROM.h>
#include "EEPROMAnything.h"


const int powerOpto=A0;
const int gray=A1;
const int purple=A2;
const int cw=10;
const int ccw=11;
const int nextGapPin=4;
const int previousGapPin=5;
const int zeroPin=3;
const int dangerZone=2000;
int counter=0;
int bottle;
long int averageSeekTime;
 long int seekTime=0;
 long int sought=0;
int incomingByte = 0; 


void setup(){
  Serial.begin(9600);
 pinMode(powerOpto,OUTPUT);
 pinMode(gray,INPUT);
 pinMode(purple,INPUT);
 pinMode(cw,OUTPUT);
 pinMode(ccw,OUTPUT);
 pinMode(nextGapPin,INPUT);
 pinMode(previousGapPin,INPUT);
 digitalWrite(cw,HIGH);
 digitalWrite(ccw,HIGH);
 digitalWrite(nextGapPin,LOW);
 //testSeekTime();
 EEPROM_readAnything(0, bottle);

 Serial.println(bottle);
 //initializeSampler();
 findZero();
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
           
            if(digitalRead(previousGapPin) &&counter==10){
             reverse();
            Serial.println(averageSeekTime);
             counter=0;
           }
            
            
            if(digitalRead(nextGapPin)||digitalRead(previousGapPin) ||digitalRead(zeroPin)){
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
          findZero();
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
  
//IF at the end of the bottles rezero, but don't advance  

    //lets try 4x to get the rotor to the next bottle
   
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
  //  Serial.println("reverse");
 //IF ALREADY AT ZERO, DON"T DO MUCH
    // if(bottle==0){
     //   Serial.println("alread at zero");
        //could add a re-zero here
    //      }
 //IF NOT AT ZERO MOVE BACK ONE BOTTLE  
   // if(bottle>0){
 //IF ON A GAP, MOVE OFF
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
  
    if (bottle<22){
         advance();
     //     advance();
      //    advance();
      moveForward(purple);
      moveForward(gray);
      moveForward(purple);
      moveForward(gray);
        }
    Serial.println("findZero");
    
    digitalWrite(ccw,LOW);
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
          if(bottle<22){
          advance();
          advance();
          averageSeekTime=floor(seekTime/sought);
          Serial.println(averageSeekTime);
          }
    
    findZero();
     
  }
  
    void moveBack(int color){
    digitalWrite(ccw,LOW);
     while(!digitalRead(color)){
       delay(20);
     }
     digitalWrite(ccw,HIGH);
  }
  
    
    void moveForward(int color){
    digitalWrite(cw,LOW);
     while(!digitalRead(color)){
       delay(20);
     }
     digitalWrite(cw,HIGH);
  }
  
void goToBottle(int target){
  if(target==1){
      findZero();
  }
  if(target!=1){
    
    while(target>bottle){
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
  
  

