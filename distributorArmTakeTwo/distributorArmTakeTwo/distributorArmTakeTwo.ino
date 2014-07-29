

const int powerOpto=A0;
const int gray=A1;
const int purple=A2;
const int cw=10;
const int ccw=11;
const int nextGapPin=4;
const int previousGapPin=5;
const int zeroPin=3;
const int pump=7;
const int pumpSwitch=2;
const int dangerZone=2000;
int counter=0;
int bottle=0;
void setup(){
  Serial.begin(9600);
 pinMode(powerOpto,OUTPUT);
 pinMode(gray,INPUT);
 pinMode(purple,INPUT);
 pinMode(cw,OUTPUT);
 pinMode(ccw,OUTPUT);
 pinMode(nextGapPin,INPUT);
 pinMode(previousGapPin,INPUT);
 pinMode(pumpSwitch,INPUT);
 pinMode(pump,OUTPUT);
 
 digitalWrite(cw,HIGH);
 digitalWrite(ccw,HIGH);
 digitalWrite(nextGapPin,LOW);
 digitalWrite(pump,HIGH);
 
 
 //initializeSampler();
 
}


void loop(){
  if(digitalRead(pumpSwitch)&&counter==10){
    Serial.println("pumping");
    while(digitalRead(pumpSwitch)){
            digitalWrite(pump,LOW);
         }
    digitalWrite(pump,HIGH);
    counter=0;
  }
  
 if(digitalRead(nextGapPin) &&counter==10){
   advance();
   counter=0;
 }
 
  if(digitalRead(zeroPin) &&counter==10){
   findZero();
   counter=0;
 }
 
  if(digitalRead(previousGapPin) &&counter==10){
   reverse();
   counter=0;
 }
  
  
  if(digitalRead(pumpSwitch)||digitalRead(nextGapPin)||digitalRead(previousGapPin) ||digitalRead(zeroPin)){
    counter++;
  }
  else {
    counter=0;
    }
 
}

void advance(){
  
  long int danger=millis();
  int bail=0;
  if (bottle>=23){
    bail=1; 
    Serial.println("already at full count");
    findZero();
    
  }
    Serial.println("advance");
    if(digitalRead(purple)==0 && !bail){//if already on a gap
      //move off of first spot
      Serial.println("onGap, move off");
      while(!digitalRead(purple) && !bail){
        digitalWrite(cw,LOW);
        if ((millis()-danger)>dangerZone){bail=1; Serial.println("bail!!");}
        }
      digitalWrite(cw,HIGH);
      Serial.println("off gap");
       Serial.println("now move to next");
    }//end if digitalRead(gray)==0;
    
   
    danger=millis();
    while(digitalRead(purple) && !bail){
          digitalWrite(cw,LOW);
          if ((millis()-danger)>dangerZone){bail=1; Serial.println("bail!!!");}
          delay(30);
    }
    digitalWrite(cw,HIGH);
    Serial.println("gotIT");
    bottle++;
    Serial.print("bottle #:"); Serial.println(bottle);
  }


void reverse(){
  long int danger=millis();
  int bail=0; 
    Serial.println("reverse");
    if(digitalRead(gray)==0){//if already on a gap
      //move off of first spot
      Serial.println("onGap, move off");
      while(!digitalRead(gray)&&!bail){
        digitalWrite(ccw,LOW);
        if((millis()-danger)>dangerZone){bail=1; Serial.println("bail!!!");}
        }
      digitalWrite(ccw,HIGH);
      Serial.println("off gap");
    }//end if digitalRead(gray)==0;
    danger=millis();
    Serial.println("now move to next");
    while(digitalRead(gray) && !bail){
          digitalWrite(ccw,LOW);
                   if ((millis()-danger)>dangerZone){bail=1;Serial.println("bail!!!");}
          delay(30);
    }
    digitalWrite(ccw,HIGH);
    Serial.println("gotIT");
        bottle--;
    Serial.print("bottle #:"); Serial.println(bottle);
  }
  
  
void findZero(){
  if (bottle==0){
    advance();
    advance();
  }
  long int danger=millis();
  int bail=0;
    Serial.println("findZero");
    if(digitalRead(gray)==0 && digitalRead(purple)==0){//may already be there
      //move off of first spot
      Serial.println("onGap, move off");
      while(!digitalRead(gray) && !bail){
        digitalWrite(ccw,LOW);
        if ((millis()-danger)>dangerZone){bail=1; Serial.println("bail!!!!");}
        }
      digitalWrite(ccw,HIGH);
      Serial.println("off gap");
    }//end if digitalRead(gray)==0;
    
    
    
    Serial.println("now move until find zero");
    while(digitalRead(gray) || digitalRead(purple)){
          digitalWrite(ccw,LOW);
          delay(30);
    }
    digitalWrite(ccw,HIGH);
    Serial.println("gotIT");
    reverse();
    reverse();
    advance();
    bottle=0;
    Serial.print("bottle #:"); Serial.println(bottle);
  }
  
  
  void initializeSampler(){
    Serial.println("initializing sampler");
    advance();
    advance();
    findZero();
     
  }
