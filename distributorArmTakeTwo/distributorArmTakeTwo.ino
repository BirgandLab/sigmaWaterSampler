#include <EEPROM.h>
#include "EEPROMAnything.h"
#include "Chronodot.h"
#include "Wire.h"
#include "SD.h"
#define chipSelect 10   


 char systemLogFile[12] = "H2O.log"; //NAME FOR THE LOG FILE WRITTEN FOR THE 
 char missionDataFile[12]="go.dat";
 String siteID="LAB";           //NAME FOR THE SITE TO BE INCLUDED IN SYSTEM 


struct missionData {
  String siteID;			//short site identification name
  ///Main Sample System Timing Variables
  int PumpTime;//Time to run pump to fill lines with fresh sample water (seconds)
  int PurgeTime;//Time to run pump (reverse) to purge lines and vessel (seconds)
  int PrimeTime;
  int sampleInterval;			//interval at which to pump to Manta system in absence of scan signal
  int startDay;
  int startHour;
  int startMinute;
  int startSecond;
  int startMonth;
  int startYear;
  int startBottle;
} missionData;

int PumpTime;//Time to run pump to fill lines with fresh sample water (seconds)
int PurgeTime;//Time to run pump (reverse) to purge lines and vessel (seconds)
int PrimeTime;
int startDay=30;
int startHour=15;
int startMinute=45;
int startSecond=0;
int startMonth=7;
int startYear=2014;
int fudge=0;
int completedCycle=0;
int startBottle=1;

static  byte monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};
#define LEAP_YEAR(_year) ((_year%4)==0)



 int samplesSinceLastPowerCycle=0;//keep track of when we lose power...just for fun
 int error=0;
long int SI=1; //minutes between samples
float VersionNumber=0.00;
long int purgeTime=4; //run pump in reverse
long int primeTime=5; //run pump forward to fill lines
long int pumpTime=3;  //run pump forward to collect into bottle


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



//ERROR KEEPING VARIABLES
int clockError=0;
int powerCycle=0;
int SDError=0;

long int startTime;
long int startTimeEEP;
long int nextSample;
long int nextSampleEEP;
int powerCycleEEP;
int sampleInterval=1; //minutes

long int firstSample;
int sampleCycleCounter=0;
int sampleCounterEEP;









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
 pinMode(chipSelect,OUTPUT);
 
 
 pinMode(pumpSwitch,INPUT);
 pinMode(gray,INPUT);
 pinMode(purple,INPUT);
 pinMode(nextGapPin,INPUT);
 pinMode(previousGapPin,INPUT);
 
 
 	//ACTIVATE THE SD CARD
        digitalWrite(chipSelect,HIGH);   //Tell the SD Card it is needed
      if (!SD.begin(chipSelect)) {//IF THE SD DOES NOT START
          Serial.println("Card failed, or not present");
        } //END  if (!SD.begin(chipSelect))

 
 EEPROM_readAnything(10, bottle);
 Serial.println(bottle);
 getSettings();
}


void loop(){
     
      if (get_unixtime()%(10)==0){           //EVERY ONCE IN A WHILE REPORT STATUS
                  timeStamp();
                 Serial.print("approximate time to next sample (seconds): ");
                 Serial.println((SI*60)-(get_unixtime()%(SI*60)));//TIME TO NEXT SAMPLE
                if (get_unixtime()%(SI*60)==0){
                          sampleRoutine();
                        //  writeSystemLogFile();
                        // samplesSinceLastPowerCycle++;
                }   //EVERY SAMPLE INTERVAL GET READY TO RUN
                  
                 //readSensors(sensorValues);  
                //writeSystemLogFile();
                //samplesSinceLastPowerCycle++;
                 delay(990); //wait so the report is only given once
               }//end get_unixtime()%(60)==0)
		
            long int elapsed=0;
            long int sought=0;

              
           if (Serial.available() > 0) {
                // read the incoming byte:
                incomingByte = Serial.parseInt();
                // say what you got:
                Serial.print("I received: ");
                Serial.println(incomingByte, DEC);
                if(incomingByte>=1 and incomingByte<=24){
                goToBottle(incomingByte);
                }
        if(digitalRead(nextGapPin)||digitalRead(previousGapPin) ||digitalRead(zeroPin) || digitalRead(pumpSwitch)){
              counter++;
            }
            else {
              counter=0;
              }
              if (counter==10){
            
                button();
              }
        }
           
}//end void loop






void button (){
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
}



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
                            error=0;
                            }
                       
                          tries++;  //augment counter, we might need to tray again
                          bail=0;    //RESET BAIL FOR ANOTHER TRY
                          if (tries>=3){
                           error=1; 
                          }
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
  
    Serial.println("gotIT");
    bottle=1;
    Serial.print("bottle #:"); Serial.println(bottle);
    EEPROM_writeAnything(10,bottle);
  }
  
  /*
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
  */
 
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
  
char* registerNames[]={"YYYY","MM","DD","HH","mm",
                          "SS","siteID","ArdID","Ver","VBat",
                          "DegC", "Bottle","powerCycle","error"};
       
char conversion[10];			        //MAKE A CONTAINER FOR CONVERSION
                   
  Serial.println("write data log file"); delay(50);
   DateTime now = RTC.now();				//PROVIDE CURRENT TIME FOR LOG FILE
   char dataString[27]; //A CONTAINER FOR THE FORMATTED DATE
   int a = sprintf(dataString,"%d\t%02d\t%02d\t%02d\t%02d\t%02d\t",now.year(),now.month(), now.day(),now.hour(),now.minute(),now.second());
   File dataFile = SD.open(systemLogFile, FILE_WRITE);
   Serial.println(dataFile);
  if (! dataFile) {
    Serial.println("error opening H2O.log");
    // Wait forever since we cant write data
    }else {
    //Serial.println(registerNames);
    if (samplesSinceLastPowerCycle==0){
 for (int j=0;j<=13;j++){							//FOR EACH LOG VARIABLE
                      dataFile.print(registerNames[j]);			//PRINT A SHORT VARIABLE NAME
                      dataFile.print("\t");      				//ADD A TAB						//ADD NEWLINE
                     
 }		
  dataFile.print("\n");	
    }				//AD
    dataFile.print(dataString);
    dataFile.print("\t");
			dataFile.print(siteID);  	//write siteID TO THE LOG FILE
			dataFile.print("\t");	 	//ADD A TAB
			
			char id[5];					//MAKE A VARIABLE FOR THE ARDUINO ID
			EEPROM_readAnything(0, id);	//READ ARDUINO ID FROM EEPROM
			dataFile.print(id);  		//write ArduinoID TO LOG FILE
			dataFile.print("\t");		//ADD A TAB
			
			dtostrf(VersionNumber,2,2,conversion); 	//CONVERT PROGRAM VERSION TO STRING
			dataFile.print(conversion);				//WRITE TO LOG FILE
			dataFile.print("\t");					//ADD A TAB
			
			dtostrf(analogRead(A3),3,4,conversion);//CONVERT VOLTAGE TO STRING
			dataFile.print(conversion);				//WRITE STRING TO LOG FILE
			dataFile.print("\t");					//ADD A TAB
		   
			dtostrf(get_tempRTC(),3,4,conversion);//CONVERT RTC DEG C TO STRING
			dataFile.print(conversion);				//WRITE STRING TO LOG FILE
			dataFile.print("\t");					//ADD A TAB
           
			dataFile.print(bottle);         //WRITE NUMBER OF FPS SAMPLES 
													//COLLECTED IN THIS INTERVAL
			dataFile.print("\t");					//ADD A TAB
           							//WRITE CYCLES SINCE LAST PC
           dataFile.print(samplesSinceLastPowerCycle);  
           dataFile.print("\t"); 					//ADD A TAB
          
	   dataFile.print(error);
            dataFile.print("\t");					//THEN ADD A TAB
            
          dataFile.print("\n");  		//add final carriage return
  
    
     dataFile.flush();
    }
dataFile.close();
  
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
 /**************************************************************************************/
 /**************************************************************************************/
 /**************************************************************************************/
 /**************************************************************************************/
 



long int makeTime(byte sec, byte min, byte hour, byte day, byte month, int year ){
// converts time components to time_t 
// note year argument is full four digit year (or digits since 2000), i.e.1975, (year 8 is 2008)
   int i;
   long int  seconds;
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
    for (i=0; i<(month-1); i++) {
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

/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
 ///READ THE IMPORTANT SYSTEM PROGRAMMING VARIABLES FROM A TEXT FILE LOCATED IN THE ROOT 
 ///DIRECTORY OF THE SD CARD.  THIS ENABLES THE USER TO NOT CONTIUNUALLY RELOAD THE FPS
 ///CORE PROGRAM ON THE ARDUINO. INSTEAD BY EDITING THE CONFIG FILE, AND RESTARTING THE
 ///SYSTEM PUMPING TIMES AND THRESHOLD VALUES CAN BE CHANGED AND TWEAKED
 
 void getSettings(){
  Serial.println("Loading Configuration");delay(30);
 // Open the settings file for reading:
   File myFile;						//DECLARE A FILE
  myFile = SD.open(missionDataFile);	
  myFile.close();	                //OPEN THE FILE
 // if(myFile){						//IF THE FILE OPENS PROCEED WITH PARSING DATA
 // Serial.println("File Opened");	delay(10);                //GIVE SOME FEEDBACK
  char character;					//AN EMPTY CHARACTER
  String description;		        	        //AND EMPTY STRING
  String value;			        	        //ANOTHER EMPTY STRING
  boolean valid = true;				        //BOOL FOR EVALUATING STATMENTS
  /*
  
          while (myFile.available()) { //READ FROM THE FILE UNTIL ITS EMPTY
            character = myFile.read(); //READ FIRST CHAR OF LINE
                if(character == '/') { //IF ITS A COMMENT READ THE WHOLE LINE
                    while(character != '\n'){
                            character = myFile.read();
                        }//END  while(character != '\n')
                   }//END if(character == '/')
				   
				//IF IT ISN'T A COMMENT, IT IS THE VARIABLE NAME
                else if(isalnum(character)) {//ADD EACH CHARACTER TO THE DESCRIPTION
                           description.concat(character);
                    } 
                else if(character =='=') {//IF IT IS AN EQUALS SIGN, 
										  //END THE VARIABLE NAME AND READ THE VALUE
                        Serial.print(description);
                        Serial.print(": ");
                          // START CHECKING THE VALUE FOR POSSIBLE RESULTS
                          // FIRST GOING TO TRIM OUT ALL TRAILING WHITE SPACES
                          do { character = myFile.read();
                                } while(character == ' ');
						  //EMPTY THE VALUE PARAMETER
                                  value = ""; 
                          //READ IN THE REST OF THE LINE
						  //WHILE THE LINE ISN'T OVER AND DATA REMAINS IN THE FILE
						  //CONTINUTE CONCATENATING CHARACTERS
								  while(character != '\n' && myFile.available()) { 
								            value.concat(character);               
                                            character = myFile.read();
                                      }//end while(character != '\n')
							//CREATE A BUFFER TO HOLD THE ASSEMBLED DATA
                                  char charBuf[value.length()+1];         
							//CONVER THE VALUE TO SOMETHING USEABLE  
                                  value.toCharArray(charBuf,value.length()+1);
                                  Serial.print(charBuf);
                                  Serial.println();
           
              //VALUE PAIR SHOULD BE CAPTURED AT THIS POINT
              //ASSIGN THEM TO REAL VARIABLES, OR TO A STRUCTURE
			  //BY MATCHING DESCRIPTION AND VARIABLE NAMES
                                  if (description == "siteID"){
                                    missionData.siteID=value;
                                  }//END  if (description == "siteID")
                                  else if (description == "PumpTime"){
                                    missionData.PumpTime=atoi(charBuf);
                                  }//END  if (description == "pumpTIme")
                                  else if (description == "PurgeTime"){
                                   missionData.PurgeTime=atoi(charBuf);
                                  } //END else if (description == "PurgeTime")
                                  else if (description == "PrimeTime"){
                                   missionData.PrimeTime=atoi(charBuf);
                                  } //END else if (description == "PrimeTime")
                                   else if (description == "PrimeTime"){
                                   missionData.PrimeTime=atoi(charBuf);
                                  } //END else if (description == "PrimeTime")
                                   else if (description == "sampleInterval"){
                                    missionData.sampleInterval=atoi(charBuf);
                                  } //END else if (description == "sampleInterval")
                                   else if (description == "startDay"){
                                    missionData.startDay=atoi(charBuf);
                                  } //END  else if (description == "startDay")
                                  else if (description == "startHour"){
                                    missionData.startHour=atoi(charBuf);
                                  } //END else if (description == "startHour")
                                  else if (description == "startMinute"){
                                   missionData.startMinute=atoi(charBuf);
                                  } //END else if (description == "startMinute")
                                   else if (description == "startSecond"){
                                    missionData.startSecond=atoi(charBuf);
                                  } //END  else if (description == "startSecond")
                                   else if (description =="startYear"){
                                    missionData.startYear=atoi(charBuf);
                                   }
                                   else if (description =="startMonth"){
                                     missionData.startMonth=atoi(charBuf);
                                   }                                  
                                   else if (description =="startBottle"){
                                    missionData.startBottle=atoi(charBuf);
                                   }
                                  else{ 
                                    Serial.println("mismatch-look for a typo");
                                  } //END ELSE
 
                                  description="";
                                       }//end   else if(character =='=')
            //reset description
    }//end while (myFile.available())
    */
 //   myFile.flush();
   // myFile.close();					// close the file:
    Serial.println("File closed");	// REPORT SUCCESS
    Serial.println("assigned parameters: ");
    delay(20);
    /*
    siteID=missionData.siteID;
    PumpTime=missionData.PumpTime;
    PurgeTime=missionData.PurgeTime;
    PrimeTime=missionData.PrimeTime;
    sampleInterval=missionData.sampleInterval;
    startDay=missionData.startDay;
    startHour=missionData.startHour;
    startMonth=missionData.startMonth;
    startYear=missionData.startYear;
    startMinute=missionData.startMinute;
    startSecond=missionData.startSecond;
    startBottle=missionData.startBottle;
    */
 // } else {
    Serial.println("didn't open!");	//REPORT FAILURE
 // } //END else
  Serial.print("siteID: ");            Serial.print(siteID);             Serial.println();
  Serial.print("PumpTime: ");          Serial.print(PumpTime);             Serial.println();
  Serial.print("PurgeTime: ");         Serial.print(PurgeTime);            Serial.println();
  Serial.print("PrimeTime: ");         Serial.print(PrimeTime);            Serial.println();
  Serial.print("SampleInterval: ");    Serial.print(sampleInterval);       Serial.println();
  Serial.print("startYear: ");         Serial.print(startYear);            Serial.println();
  Serial.print("startMonth: ");        Serial.print(startMonth);           Serial.println();
  Serial.print("startDay: ");          Serial.print(startDay);             Serial.println();
  Serial.print("startHour: ");         Serial.print(startHour);            Serial.println();
  Serial.print("startMinute: ");       Serial.print(startMinute);          Serial.println();
  Serial.print("startSecond: ");       Serial.print(startSecond);          Serial.println();
  Serial.print("startBottle: ");       Serial.print(startBottle);          Serial.println();
  delay(20);
  }//end getSettings() FUNCTION
  
  
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

void difference(long int one, long int two){    //two is "now" and one is nextSample
    int hours=floor((one-two)/3600);
    int minutes=floor(((one-two)%3600)/60);
    int seconds=(one-two)%60;
    char countDown[10];
    int a=sprintf(countDown,"%02d:%02d:%02d",hours,minutes,seconds);
    
   Serial.println(countDown);
    }
    
