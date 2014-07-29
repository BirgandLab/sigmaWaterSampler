#include <EEPROM.h>
#include "EEPROMAnything.h"
#include "Chronodot.h"
#include "Wire.h"
#include "SD.h"



 char systemLogFile[12] = "water.log"; //NAME FOR THE LOG FILE WRITTEN FOR THE 
 char siteID[10] ="LAB";           //NAME FOR THE SITE TO BE INCLUDED IN SYSTEM 
#define chipSelect 10   
 int samplesSinceLastPowerCycle=0;//keep track of when we lose power...just for fun
 int error=0;
int SI=1; //minutes between samples
float VersionNumber=0.00;
long int purgeTime=0; //run pump in reverse
long int primeTime=0; //run pump forward to fill lines
long int pumpTime=0;  //run pump forward to collect into bottle


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

const int pump=            6;    //pump motor empty lines
const int purge=             7;     //pump motor fill lines and bottles
const int cw=               8;       //turn distributor arm clockwise
const int ccw=              9;      //turn distributor arm counter clockwise


const int dangerZone=2000;
int counter=0;
int bottle;
long int averageSeekTime;
 long int seekTime=0;
 long int sought=0;
int incomingByte = 0; 
Chronodot RTC;

void setup(){
  Serial.begin(9600);
  Wire.begin();              //turn on i2c bus
  RTC.begin();               //turn the clock interface on
  
// OUTPUTS
 pinMode(powerOpto,OUTPUT);
 pinMode(pump,OUTPUT);
 digitalWrite(pump,HIGH);
 pinMode(purge,OUTPUT);
 digitalWrite(purge,HIGH);
 pinMode(cw,OUTPUT);
 pinMode(ccw,OUTPUT);
 digitalWrite(cw,HIGH);
 digitalWrite(ccw,HIGH);
 
 
 pinMode(pumpSwitch,INPUT);
 pinMode(gray,INPUT);
 pinMode(purple,INPUT);
 pinMode(nextGapPin,INPUT);
 pinMode(previousGapPin,INPUT);
 
 
 	//ACTIVATE THE SD CARD
    digitalWrite(chipSelect,HIGH);   //Tell the SD Card it is needed
  
      if (!SD.begin(chipSelect)) {//IF THE SD DOES NOT START
          Serial.println("Card failed, or not present");
          //errorCodes[0]=1;  //add error to code, 
							//(although, it won't help, as they won't get logged ......
          //return;
        } //END  if (!SD.begin(chipSelect))

 //testSeekTime();
 EEPROM_readAnything(10, bottle);

 Serial.println(bottle);
 //initializeSampler();
 //findZero();
 //EEPROM_writeAnything(10,bottle);
}


void loop(){
     
      if (get_unixtime()%(60)==0){           //EVERY ONCE IN A WHILE REPORT STATUS
                 Serial.print("approximate time to next sample: ");
                 Serial.println(SI-get_unixtime()%(SI*60)/60);//TIME TO NEXT SAMPLE
                if (get_unixtime()%(SI*60)==0){
                          sampleRoutine();
                         // writeSystemLogFile();
                }   //EVERY SAMPLE INTERVAL GET READY TO RUN
                 timeStamp(); 
                 //readSensors(sensorValues);  

                 delay(900); //wait so the report is only given once
               }//end get_unixtime()%(60)==0)
		
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

void sampleRoutine(){
   //PURGE LINE TO MAKE SURE IT IS EMPTY
   Serial.println("purge");
        systemPump(purge,purgeTime);
        delay(300);
   //PUMP TO PRIME LINES 
  Serial.println("prime");
        systemPump(pump,primeTime);   
       delay(300); 
   //PUMP INTO BOTTLE  
  Serial.println("pump");
        systemPump(pump,pumpTime);
        delay(300);
    //PURGE LINE TO MAKE SURE IT IS EMPTY
   Serial.println("purge");
        systemPump(purge,purgeTime);
   Serial.println("write log file");
        writeSystemLogFile();
   Serial.println("advance distributor arm");
        advance();
        samplesSinceLastPowerCycle++;
}

void systemPump(int Direction, long int duration){
         digitalWrite(Direction,LOW);
              long int timer=millis();
              while ((millis()-timer)<(duration*1000)){
                //wait;
              }
        digitalWrite(Direction,HIGH); 
}

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
    EEPROM_writeAnything(10,bottle);                          //THE BOTTLE NUMBER WE THINK IT IS ON NOW
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
    EEPROM_writeAnything(10,bottle);                          //THE BOTTLE NUMBER WE THINK IT IS ON NOW
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
    EEPROM_writeAnything(10,bottle);
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
    EEPROM_writeAnything(10,bottle);
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
  

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
///PROVIDES SERIAL OUTPUT ABOUT TIME, TEMPERTURE AND THE LIKE
///GOOD FOR MAKING SURE RTC IS RUNNING AND CYCLES ARE EXECUTING ACCORDING TO PLAN
void timeStamp(){
  DateTime  now = RTC.now();         //start clock object "now"
  //Serial.println(timestamp);
 Serial.print(now.year(), DEC);
    Serial.print('/');
    if(now.month() < 10) Serial.print("0");
    Serial.print(now.month(), DEC);
    Serial.print('/');
    if(now.day() < 10) Serial.print("0");
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    if(now.hour() < 10) Serial.print("0");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    if(now.minute() < 10) Serial.print("0");
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    if(now.second() < 10) Serial.print("0");
    Serial.print(now.second(), DEC);
    Serial.print("\t");

    Serial.print(now.tempC(), 1);
    Serial.println(" degrees Celcius");
    //Serial.print(now.tempF(), DEC);
    //Serial.println(" degrees Farenheit");
    
    Serial.println();
}//END timeStamp() FUNCTION

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
///WRITE DATA FROM ARDUINO SYSTEM TO THE LOG FILE THIS INCLUDES VARIABLES USED TO MAKE 
///FPS COLLECTION DECISIONS, SYSTEM STATUS, TIME, RESTARTING, BATTERY LEVEL, TEMP
///CUMULATIVE FLOW ETC. THUS IT WILL BE GOOD FOR TROUBLESHOOTING

void writeSystemLogFile(){
//PREPARED DATA FOR FILE HEADER ABBREVIATIONS OF VARIABLES AND UNITS ARE HERE 
//EVERYTHING IS TAB DELIMITED, SO EXCEL SHOULD READ IT IN WELL
//MUST BE CHANGED IF THE OUTPUTS ARE CHANGED

    char* registerNames[]={"YYYY","MM","DD","HH","mm",
                          "SS","siteID","ArdID","Ver","VBat",
                          "DegC", "Bottle","error","powerCycle"};
    char* units[]={"[YYYY]","[MM]","[DD]","[HH]","[mm]",
                   "[SS]","[ID]","[ARD]","[Ver]","[v]",
                   "[degC]","[#]","[0/1]","[0/1]"};
                          
        //File myFile = SD.open(systemLogFile, FILE_WRITE);
  
	if (!SD.exists(systemLogFile)){ 
        Serial.println("make a new log file");					//IF THE LOG FILE DOES NOT EXIST
		File dataFile=SD.open(systemLogFile,FILE_WRITE);//CREATE THE LOG FILE
        
                for (int j=0;j<=13;j++){							//FOR EACH LOG VARIABLE
                      dataFile.print(registerNames[j]);			//PRINT A SHORT VARIABLE NAME
                      dataFile.print("\t");      				//ADD A TAB
                      }//END for (int j=0;j<30;j++)
                      
        		dataFile.print("\n");							//ADD NEWLINE
        		
                for (int j=0;j<13;j++){							//FOR EACH LOG VARIABLE
                      dataFile.print(units[j]);					//PRINT UNITS
                      dataFile.print("\t");      				//ADD A TAB
                      } //END  for (int j=0;j<30;j++)
                      
               dataFile.print("\n");							//ADD A NEWLINE
               dataFile.close();   								//CLOSE THE FILE
       }//END 	if (!SD.exists(systemLogFile))
  
 
      char conversion[10];			        //MAKE A CONTAINER FOR CONVERSION
	
      DateTime now = RTC.now();				//PROVIDE CURRENT TIME FOR LOG FILE
  
      File dataFile=SD.open(systemLogFile, FILE_WRITE);   //open log file	  
	
          		 //IF THE DATA FILE OPENED WRITE DATA INTO IT
			char dataString[27]; //A CONTAINER FOR THE FORMATTED DATE
			dataFile.print("boogers");
			//PRINT THE DATE (YYYY MM DD HH mm SS) TO dateString variable
			int a = sprintf(dataString,"%d\t%02d\t%02d\t%02d\t%02d\t%02d\t",now.year(), 
				now.month(), now.day(),now.hour(),now.minute(),now.second());
			
				
			dataFile.print(dataString);	//WRITE THE DATE STRING TO THE LOG FILE	
	/*		
			dataFile.print(siteID);  	//write siteID TO THE LOG FILE
			dataFile.print("\t");	 	//ADD A TAB
			
			char id[5];					//MAKE A VARIABLE FOR THE ARDUINO ID
			EEPROM_readAnything(0, id);	//READ ARDUINO ID FROM EEPROM
			dataFile.print(id);  		//write ArduinoID TO LOG FILE
			dataFile.print("\t");		//ADD A TAB
			
			//dtostrf(VersionNumber,2,2,conversion); 	//CONVERT PROGRAM VERSION TO STRING
			dataFile.print(conversion);				//WRITE TO LOG FILE
			dataFile.print("\t");					//ADD A TAB
			
			//dtostrf(analogRead(A3),3,4,conversion);//CONVERT VOLTAGE TO STRING
			dataFile.print(conversion);				//WRITE STRING TO LOG FILE
			dataFile.print("\t");					//ADD A TAB
		   
			//dtostrf(get_tempRTC(),3,4,conversion);//CONVERT RTC DEG C TO STRING
			dataFile.print(conversion);				//WRITE STRING TO LOG FILE
			dataFile.print("\t");					//ADD A TAB
           
			dataFile.print(bottle);         //WRITE NUMBER OF FPS SAMPLES 
													//COLLECTED IN THIS INTERVAL
			dataFile.print("\t");					//ADD A TAB
           							//WRITE CYCLES SINCE LAST PC
           dataFile.print(samplesSinceLastPowerCycle);  
           dataFile.print("\t"); 					//ADD A TAB
          
	   //dataFile.print(error);
            dataFile.print("\t");					//THEN ADD A TAB
            
          dataFile.print("\n");  		//add final carriage return
         */
          dataFile.close();				//close file
         
          Serial.println("successfuADFALSDFJASile Writing");
         // Serial.println(dataString);
         
	//	}//END if (dataFile)
    
  //Serial.println("successful File Writing");
 
 
}//END void writeSystemLogFile(float waterDepth, float collect, int numberOfSamples)



/**************************************************************************************/
/***************** FUNCTION TO READ UNIXTIME FROM RTC *********************************/
/**************************************************************************************/

unsigned long int get_unixtime(){
    DateTime  now = RTC.now();  //DECLARE A RTC OBJECT
	unsigned long int time = now.unixtime();
	return time;			//RETURN  TIME IN SECONDS SINCE 1/1/1970 00:00:00
}//END unsigned long int get_unixTime()

/**************************************************************************************/
/************* FUNCTION TO READ TEMPERATURE IN C FROM RTC *****************************/
/**************************************************************************************/

float get_tempRTC(){
	DateTime now = RTC.now();//DECLARE A RTC OBJECT
	float tempC = now.tempC();
	return tempC;			//RETURN THE TEMPERATURE
	
}//END float get_tempRTC()

 /**************************************************************************************/
/*********** FUNCTION TO READ BATTERY VOLATE FROM VOLTAGE DIVIDER *********************/
/**************************************************************************************/
 
float get_vBatt(){
 float vBatt = (analogRead(VBat));	//READ BATTERY VALUE
 float vBattF = (vBatt/1023*14.4*1.1);
 return vBattF;			//RETURN CONVERTED TO VOLTS
}//END float get_vBatt()

/**************************************************************************************/
/**************** FUNCTION TO READ STATUS OF FLOAT SWITCH *****************************/
/**************************************************************************************/
/*void convertTimeToEunuch(byte sec, byte min, byte hour, byte day, byte month, int year ){
// converts time components to time_t 
// note year argument is full four digit year (or digits since 2000), i.e.1975, (year 8 is 2008)
  
   int i;
   time_t seconds;

   if(year < 69) 
      year+= 2000;
    // seconds from 1970 till 1 jan 00:00:00 this year
    seconds= (year-1970)*(60*60*24L*365);

    // add extra days for leap years
    for (i=1970; i<year; i++) {
        if (LEAP_YEAR(i)) {
            seconds+= 60*60*24L;
        }
    }
    // add days for this year
    for (i=0; i<month; i++) {
      if (i==1 && LEAP_YEAR(year)) { 
        seconds+= 60*60*24L*29;
      } else {
        seconds+= 60*60*24L*monthDays[i];
      }
    }

    seconds+= (day-1)*3600*24L;
    seconds+= hour*3600L;
    seconds+= min*60L;
    seconds+= sec;
    return seconds; 
}
*/
