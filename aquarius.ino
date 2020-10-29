// SD - Version: Latest 
#include <SD.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SETWARNING(V) digitalWrite(13,(V))

#define ONE_WIRE_BUS 2
#define SLOG "slog.txt"//8 char
#define CSVPREFIX "periodo"//3 char
#define LAPSE 5//minutes
#define PERIOD 48//hours

typedef struct stTemp{
  float t;
  short m;
} temp;

//prototypes
void slog(unsigned v);
void slog(char v);
void slog(char* s);
void wait(unsigned long milliseconds);
void log(temp t);
temp getTemperature(short minute);
bool sdInitializer();
bool sdWriter(bool mode);
void uiFiller(short to,char symbol,char* s,bool log=true);

//to avoid some repetitive declarations
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
File file;
short i,minutes=PERIOD*60,incVal=1;
temp temps[32];
char incName[10],sbuffer[256]={0};

void setup(){
  pinMode(13,OUTPUT);   //sets the status led
  for(i=0;i<32;i++)
    temps[i].t=-127;
  
  Serial.begin(9600);   //initialize serial
  while(!Serial);       //wait for serial port to connect. Needed for native USB port only

  Serial.print("################################\r\n#     Aquarius Temp Logger     #\r\n#          by Lucide           #\r\n#            v1.0              #\r\n");
  slog("#         ..booting..          #\r\n");

  sensors.begin();
  sensors.setResolution(11);
  sdInitializer();
}

void loop(){
  if(minutes==PERIOD*60){//if 48h has passed
    slog("#      move to ");
    sprintf(incName,"%.3s%02d.csv",CSVPREFIX,incVal);
    slog(incName);
    slog("       #\r\n");

    incVal++;
    minutes=-LAPSE;
  }
  minutes+=LAPSE;
  
  slog("#            idle              #\r\n");
  wait(LAPSE*60000-1000);//one minute in milliseconds
  SETWARNING(HIGH);
  wait(1000);
  log(getTemperature(minutes));
  SETWARNING(LOW);
}

//slog (system log) functions' objective is to print the message and
//put a slimmed down version on a temporary buffer,
//waiting to be written on the sd card
void slog(short v){
  short t;
  char s[10];
  snprintf(s,10,"%d",v);
  t=strlen(sbuffer);
  Serial.print(s);
  snprintf(sbuffer+t,(255-t<2)?0:255-t,"%s",s);
}
void slog(char v){
  short t=strlen(sbuffer);
  Serial.print(v);
  if(v!=' '&&v!='#'&&t<255){
    sbuffer[t]=v;
    sbuffer[t+1]=0;
  }
}
void slog(char* s){
  short t;
  Serial.print(s);
  for(t=0;s[t]&&(s[t]==' '||s[t]=='#');t++);
  s+=t;
  t=strlen(sbuffer);
  snprintf(sbuffer+t,(255-t<2)?0:255-t,"%s",s);
  s=strrchr(sbuffer+t,'#');

  if(s){
    for(t=s-sbuffer-1;t>-1&&sbuffer[t]==' ';t--);
    sbuffer[t+1]='\r';
    sbuffer[t+2]='\n';
    sbuffer[t+3]=0;
  }
}

void wait(unsigned long milliseconds){
  unsigned long start=millis();
  while(millis()-start<milliseconds);
}

void log(temp t){
  char s[10];

  Serial.print("\0");//holds everything together :[

  dtostrf(t.t,0,2,s);
  Serial.print("< ");
  Serial.print(t.m);
  Serial.print(". temp: ");
  uiFiller(20,'>',s,false);
  if(!sdWriter(true)){
    slog("#    launching re-inix card    #\r\n");
    if(sdInitializer()){
      sdWriter(true);
      sdWriter(false);
    }
  }
  else
    sdWriter(false);
//Serial.println('[');
//Serial.println(sbuffer);
//Serial.println(']');
}

temp getTemperature(short minute){
  temp t;

  slog("# attempt to read temperature  #\r\n");
  sensors.requestTemperatures();
  for(i=1;i<4&&(t.t=sensors.getTempCByIndex(0))==-127;i++){
    slog("#      ..failed, try ");
    slog(i);
    slog("..       #\r\n");
    delay(1000);
    sensors.requestTemperatures();
  }
  if(i==4){
    slog("#    attempt dropped, -127     #\r\n");
  }
  t.m=minute;
  for(i=0;i<32&&temps[i].t!=-127;i++);
  if(i<32)
    temps[i]=t;
  return t;
}

bool sdInitializer(){
  slog("#  attempt to initialize card  #\r\n");
  for(i=1;i<4&&!SD.begin(4);i++){
    slog("#      ..failed, try ");
    slog(i);
    slog("..       #\r\n");
    delay(1000);
  }
  if(i<4){
    slog("#       card initialized       #\r\n");
    return true;
  }
  else{
    slog("#      card not present,       #\r\n");
    slog("# running in serial-only mode  #\r\n");
    return false;
  }
}

bool sdWriter(bool mode){
  char *fileName,t1[20],t2[10];

  if(mode)
    fileName=incName;
  else
    fileName=SLOG;
  slog("#  attempt to open ");
  uiFiller(12,'#',fileName);
  for(i=1;i<3&&!(file=SD.open(fileName,FILE_WRITE));i++){
    slog("#      ..failed, try ");
    slog(i);
    slog("..       #\r\n");
    delay(1000);
  }
  if(i==3){
    slog("#       attempt dropped        #\r\n");
    return false;
  }
  else{
    if(mode){
      for(i=0;i<32&&temps[i].t!=-127;i++){
        dtostrf(temps[i].t,0,2,t2);
        *(strrchr(t2,'.'))=',';
        snprintf(t1,19,"%d;%s",temps[i].m,t2);
        file.println(t1);
      }
      for(i=0;i<32;i++)
        temps[i].t=-127;
    }
    else{
      file.println(sbuffer);
      sbuffer[0]=0;
    }
    file.close();
    slog("#      attempt succeeded       #\r\n");
    return true;
  }
}

void uiFiller(short to,char symbol,char* s,bool log){
    if(log)
      slog(s);
    else
      Serial.print(s);
    for(i=strlen(s);i<to;Serial.print(' '),i++);
    Serial.print(symbol);
    slog("\r\n");
}



