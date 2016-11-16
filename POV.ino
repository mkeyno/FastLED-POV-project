
 
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h> 
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <Hash.h>
 
 
 
#include <FastLED.h>
#define HalSensor 14
#define NUM_LEDS  144 //number of leds in strip length on one side
#define DATA_PIN    3 // (8,15),( 4,2) (6,12) (7,13) 
                      // (2,4)   (1,5) (5,14) (0,16)
#define CLOCK_PIN  4  //(5,14)


static byte L=0;
File FILe;

CRGB leds[NUM_LEDS];
 static uint8_t * _4kbuffer;
bool StartUpShow=true,SHOW=false,Start_new_round=false;
String IMAGES[24]={};
byte  IMAGES_Prior[24]={0,1,2,3,4},IMAGE_NUM=0;
int DURATION[24]={5,5,5,5,5};
const int IMAGES_LINES=200;
static int image_index=0, Current_imageLine=0;
 
volatile uint32_t t_start_round=0,t_stop_round=0,OpenlastTime=0,lastLineShow=0,lastFrameShow=0;
volatile uint32_t lineInterval=400;


 
ESP8266WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

 
IPAddress apIP(192, 168, 4, 1),_ip;


String MyMode=""; // add 0
String myHostname = ""; //add 1-14
String SSID=""; //add 15-29
String SSIDPASS="";//add 30-44
String AP_SSID="";//add 45-59
String AP_SSIDPASS="";//add 60-74 
 


 
String MyIP="";
 

String uploadError="";  
String PATH="/";
 
unsigned long Ego_Seconds=27000;

byte client_num=0;

String read_line(String _path,byte line_num){
File f = SPIFFS.open(_path, "r");
if (!f) return "" ; 
byte counter=0; 
String line="";
while(f.available())//1AAA@THAT54545646|1$15:
{ 
 line = f.readStringUntil('\n'); 
 line.trim();
 //Serial.print("read line="); Serial.println(line);                          
 if(line.length() >2 && counter==line_num) break;
 counter++;
}
 f.close();  
 return line;
}
 
 
String Retrive_file(String _File,String perfix)  
{
File f = SPIFFS.open("/data/"+_File, "r");  
  if (!f) return ""; 
String content="";
 while(f.available()) { 
                        char c=f.read(); if(c=='\n')  break;
                        content+=c;
                     }
content.trim();
 return  perfix+"$"+content;                     
}
 
void Rotate_1_round(void)
{
  cli();
  t_stop_round=micros();
   uint32_t   ST_L=(t_stop_round-t_start_round)/(IMAGES_LINES);//spesific time for each line
  lineInterval=(lineInterval+ST_L)/2;  
  t_start_round=t_stop_round;
  Start_new_round=true;
  //Serial.println("Interrupt");
  sei();
}
bool read_line(uint16_t _line_num,byte _Image_index)
{
  String path="/SHOW/",line="";
  uint16_t char_count=0;
  byte r=0,g=0,b=0,rgb=0,i=0;
  path+=IMAGES[_Image_index];
   Serial.print("line num is: "); Serial.println(_line_num);  
  File f = SPIFFS.open(path, "r"); 
  if(!f){ Serial.println("NOT OPEN");return false;}
   while(f.available()) { 
                            char c=f.read(); // Serial.print(c);
                             if( char_count >=(     _line_num*(NUM_LEDS*3+3)   ) ){ //line+=c; 
                                                                                                   if(rgb==0) r=byte(c);
                                                                                              else if(rgb==1) g=byte(c);
                                                                                              else if(rgb==2) b=byte(c);
                                                                                              else           { rgb=0;
                                                                                                               Serial.printf("R:%d , G:%d , B:%d \n",r,g,b);
                                                                                                               leds[i] = CRGB(r,g,b);
                                                                                                               i++; 
                                                                                                               }
                                                                                              rgb++;                             
                                                                                        }
                             if( char_count > (( _line_num+1)*(NUM_LEDS*3+3)-3 ) ) break;
                           char_count++;
                          
              }
   f.close();   
  //Serial.printf("\nline: %s\n",line.c_str());
  return true;
}

void sort_image(void)
{
  String copy="";
  byte   due=0,pri=0;
  for(int i=0;i<IMAGE_NUM;i++) IMAGES_Prior[i]=(byte)EEPROM.read(i+20);
  for(int i=0;i<IMAGE_NUM;i++)     DURATION[i]=(byte)EEPROM.read(i+70);
  for(int i=0;i<IMAGE_NUM;i++){
                for(int j=0;j<IMAGE_NUM-1;j++)
                              {
                                if(IMAGES_Prior[j]>IMAGES_Prior[j+1])
                                                  {
                                                    
                                                    copy=IMAGES[j+1];
                                                    due=DURATION[j+1];
                                                    pri=IMAGES_Prior[j+1];
                                                      IMAGES[j+1]=IMAGES[j];
                                                     DURATION[j+1]=DURATION[j];
                                                     IMAGES_Prior[j+1]=IMAGES_Prior[j];
                                                   IMAGES[j]=copy;
                                                   DURATION[j]=due;
                                                   IMAGES_Prior[j]=pri;
                                                  
                                                  }
                               
                              }
              }
  for(int i=0;i<IMAGE_NUM;i++) Serial.printf("IMAGES[%d]=%s , priority=%d, Duration=%d \n",i,IMAGES[i].c_str(),IMAGES_Prior[i],DURATION[i]);
}
void showActiveImage(void)
{         
  IMAGE_NUM=read_image("/SHOW");
  sort_image();
  
  String records="<table style=\"position:absolute;color:#FFFFFF;width:400px;font-size:13px; border: 0px #000000 none;\" id=\"mytable\">";
           records+="<tr>";
       records+="<td><strong>Num:</strong></td>";
            records+="<td><strong>Image Name</strong></td>";
                       records+="<td><strong>Priority </strong></td>";
                                 records+="<td><strong>Duration</strong></td>";                                                   
       records+="<\tr>";
  for(int i=0;i<IMAGE_NUM;i++)
              { 
                         
                             records+="<tr>";   
                 records+="<td>"+ String(i) +"</td>";                             
                 records+="<td>"+IMAGES[i]+"</td>";
                 records+="<td>"+  String(IMAGES_Prior[i]) +   "</td>";
                 records+="<td>"+  String(DURATION[i]) +   "</td>";
                             records+="<\tr>";              
              }   
 records+="</table>";
 server.send(200, "text/plain", records); 
}
/////////////////////////////////              webSocket              ///////////////////////////////////////
''''''''''''''''''''''''''''''''''' setting page ''''''''''''''''''''''''''''''''''''''''''''''
void display(String command){
  (command[As+1]=='1')?SHOW=true:SHOW=false;
} 
   

 
/////////////////////////////////              webSocket              ///////////////////////////////////////
String formatBytes(size_t bytes){
       if (bytes < 1024)                 return String(bytes)+"B"; 
  else if(bytes < (1024 * 1024))         return String(bytes/1024.0)+"KB";
  else if(bytes < (1024 * 1024 * 1024))  return String(bytes/1024.0/1024.0)+"MB";
  else                                   return String(bytes/1024.0/1024.0/1024.0)+"GB";
}
String getContentType(String filename){
       if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
void DIR(void){
  uint32_t total=0;
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
              String fileName = dir.fileName();
              size_t fileSize = dir.fileSize();
                    total+=fileSize;
              Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
            }
    Serial.print("\t\t\tTotal:");Serial.println(total);
  
}
void send_My_IP(void){
  if(WiFi.getMode()==WIFI_AP) server.send(200, "text/plain", WiFi.softAPIP().toString());
  else                        server.send(200, "text/plain",  WiFi.localIP().toString());
}
 
void removefile(String path, String filename){
  if(!SPIFFS.exists(path+filename))  return server.send(404, "text/plain", "FileNotFound");  
  SPIFFS.remove(path+filename);             server.send(200, "text/plain", "removed");
}
 

void Show_Image_List(void)
{
 String records="<select name=\"Combobox1\" width=\"340\" size=\"20\" id=\"listfile\"  onChange=\"parse_line();\"   >";
   byte i=0;
   Dir dir = SPIFFS.openDir("/SHOW");
    while (dir.next()) {    
              String fileName = dir.fileName();
              size_t fileSize = dir.fileSize();
              records+="<option value=\"" + String(i)+"\">" +fileName +"</option>";
              i++;
            }
  records+="</select>";
 server.send(200, "text/plain", records);   
  
}
void show_listfile(void){ ///replace all item of list box
   String records="<select name=\"Combobox1\" width=\"340\" size=\"20\" id=\"listfile\"  onChange=\"parse_line();\"   >";
   byte i=0;
   Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {    
              String fileName = dir.fileName();
              size_t fileSize = dir.fileSize();
              records+="<option value=\"" + String(i)+"\">" +fileName +"</option>";
              i++;
            }
  records+="</select>";
 server.send(200, "text/plain", records);   
}
 
//////////////////////////////////////////////////////////////////
byte read_image(String _path){
    Dir dir = SPIFFS.openDir(_path);
    int j=0;
    while (dir.next()) IMAGES[j++] = dir.fileName().substring(6);    
    Serial.printf("sizeof IMAGES: %d\n",sizeof(IMAGES));
    for(int i=0;i<j;i++) Serial.printf("Image File=%s\n",IMAGES[i].c_str());
  return j;    
}
void update_show_list(void){
  IMAGE_NUM=read_image("/SHOW");
  sort_image();
  String records="<table style=\"position:absolute;color:#FFFFFF;width:600px;font-size:13px; border: 2px #000000 none;\" id=\"mytable\">";
           records+="<tr>";
       records+="<td><strong>Num:</strong></td>";
            records+="<td><strong>Image Name</strong></td>";
                       records+="<td><strong>Priority </strong></td>";
                                 records+="<td><strong>Duration</strong></td>"; 
                                                    records+="<td><strong>Set</strong></td>";
       records+="<\tr>";
  for(int i=0;i<IMAGE_NUM;i++)
              { 
                         
                             records+="<tr>";   
                 records+="<td>"+ String(i) +"</td>";                             
                 records+="<td>"+IMAGES[i]+"</td>";
                 records+="<td> <input type=\"number\" id=\"PB_" + String(i )+"\" style=\"width:37px;height:18px;\" name=\"EBP\" value=\"" +IMAGES_Prior[i] +"\"> </td>";
                 records+="<td> <input type=\"number\" id=\"DB_" + String(i) +"\" style=\"width:37px;height:18px;\" name=\"EBD\" value=\"" +DURATION[i] +"\"> </td>";
                 records+="<td> <input type=\"button\"  id=\"B_" + String(i) +"\" style=\"width:59px;height:25px;\" onclick=\"SetConfig(this.id);return false;\" name=\"\" value=\"Set\"> </td>";                 
                             records+="<\tr>";              
              }   
 records+="</table>";
 server.send(200, "text/plain", records);
}
 
 
 
/////////////////////////////////////////////////////////////////
void process() // this for httml request
{
  Serial.print("[HTML]"); Serial.print(server.arg(0)); Serial.print("  ACCESS="); Serial.println(ACCESS); 
   
       if(server.arg(0)=="WSS") send_My_IP();  
  else if(server.arg(0)=="IMG") Show_Image_List() ;// CONFIG  
  else if(server.arg(0)=="SLF") show_listfile() ;// CONFIG  
  else if(server.arg(0)=="SHO") display(server.arg(1)); 
  else if(server.arg(0)=="SFC") show_file_content(server.arg(1),server.arg(2)) ; 
  else if(server.arg(0)=="USL") update_show_list(); 
  else if(server.arg(0)=="RUL") show_user_list("/data/user.dat","ListUserListBox","parse_line_and_put_inform"); 
 else if(server.arg(0)[1]=='S' && server.arg(0)[2]=='T') {server.send(200, "text/plain", ""); delay(500); ESP.restart();}  
   
  }

 
bool handleFileRead(String path){///>>>>>>>>> Authentification before sending page
   Serial.print(" Request page : " + path); 
  if(path.endsWith("/")) path += "index.html";  
  String contentType = getContentType(path);   
  String pathWithGz = path + ".gz"; 
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
                             if(SPIFFS.exists(pathWithGz))  path += ".gz";                           
                            File file = SPIFFS.open(path, "r");
                            size_t sent = server.streamFile(file, contentType);///>>>>>>>>>
                            file.close();
                            Serial.println("\t[SENT]");
                            return true;
                            }
  Serial.println("\t[Not Found]");
  return false;
}
void handleFileUploadImage(){ 
  if(server.uri() != "/updateImage" ) return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
                      String filename = upload.filename;                            
                       if(!filename.startsWith("/")) filename = "/SHOW/"+filename; 
                       //if(PATH.length()>3) filename=PATH+filename;
                       
                      if (SPIFFS.exists(filename) )    SPIFFS.rename(filename,(filename+".BAK"));
                      Serial.print("handleFileUpload Name: "); Serial.println(filename);
                      fsUploadFile = SPIFFS.open(filename, "w");
                      filename = String();
                      } 
  else if(upload.status == UPLOAD_FILE_WRITE){
                        //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
                        if(fsUploadFile)      fsUploadFile.write(upload.buf, upload.currentSize);
                        } 
  else if(upload.status == UPLOAD_FILE_END){
                      if(fsUploadFile)      fsUploadFile.close();
                      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
                      }
}
 

void handleRoot() {  
     handleFileRead("/Sindex.html");   
}

void handleOther(){
 if(!handleFileRead(server.uri()))
  {
    //Serial.println("Request other files");     
    String  message = "File Not Found\n\n";
        message += "URI: ";
        message += server.uri();
        message += "\nMethod: ";
        message += (server.method() == HTTP_GET)?"GET":"POST";
        message += "\nArguments: ";
        message += server.args();
        message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message); 
  }  
}

 

 void readFile(String testFile){ 
 Serial.print("Read File \""+testFile); 
 uint32_t startTime = millis();
 File f = SPIFFS.open("/SHOW/"+testFile, "r");
 if(!f) { Serial.println("Could not open file for reading"); return; } 
size_t i = f.size();
 Serial.printf("\",  File size is %u Bytes\n", i); 
 while(i > 3888){
         f.read(_4kbuffer, 3888);
         optimistic_yield(10000); 
         Serial.print("."); i -= 3888;
         } 
 f.read(_4kbuffer, i); 
 
 f.close(); 
 uint32_t timeTaken = millis() - startTime;
 Serial.printf(" Reading took %u ms\n\n", timeTaken); 
 } 



void setup() {

  
   // Serial.begin(115200);
 Serial.begin(115200, SERIAL_8N1,SERIAL_TX_ONLY);
  delay(500);
    Serial.setDebugOutput(true);
  Serial.println(F("\n \nStarting.....")); 
delay(500);
 SPIFFS.begin();
 
  FastLED.addLeds<APA102,DATA_PIN,CLOCK_PIN,RGB,DATA_RATE_MHZ(20)>(leds,NUM_LEDS);
   FastLED.setBrightness(255);
  FastLED.clear();
 FastLED.show();  
  DIR(); 
 
 
   
 IMAGE_NUM=read_image("/SHOW");
  sort_image();
 
WiFi.mode(WIFI_AP);
WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
WiFi.softAP(AP_SSID.c_str(),AP_SSIDPASS.c_str());                                
delay(500); // Without delay I've seen the IP address blank

MyIP=WiFi.softAPIP().toString();
Serial.print("AP IP address: ");Serial.println(MyIP.c_str());
 
   server.on("/updateImage", HTTP_POST, [](){ Serial.println("Upload post");    server.send(200, "text/plain", "Loaded"); /* Redirect("/setting.html");*/  }, handleFileUploadImage);
  server.on("/", handleRoot);
   
 
  server.onNotFound(handleOther);
  
  server.begin();  Serial.println(F("\t\t\tHTTP server started"));  
  
  const char * headerkeys[] = {"User-Agent","Cookie"} ; //here the list of headers to be recorded
  size_t hz = sizeof(headerkeys)/sizeof(char*); 
  server.collectHeaders(headerkeys, hz ); //ask server to track these headers
  
 
 attachInterrupt(digitalPinToInterrupt(HalSensor), Rotate_1_round, FALLING);
 OpenlastTime=millis();


 _4kbuffer = (uint8_t *)malloc(3888); if(_4kbuffer == NULL){ Serial.println("could not malloc payload"); while(1) delay(1000); }  

 for(byte i=0; i<IMAGE_NUM;i++) readFile(IMAGES[i]);
 
}



bool finish_chunk=true;

void loop() {

 if(!SHOW) server.handleClient();  
  webSocket.loop();
 
  if(SHOW)
      {
        if( (millis()- OpenlastTime) > DURATION[image_index]*1000L )
        {
          Serial.print(F("duration="));  Serial.print(millis()- OpenlastTime);
          FILe.close();
          image_index++; 
          if(image_index>IMAGE_NUM)    image_index=0;
          
           Serial.print(F(" i="));  Serial.print(image_index); 
          Serial.print(F(" New Image="));  Serial.println(IMAGES[image_index]);
          //Serial.print(F(" duration="));  Serial.println(DURATION[image_index]);
           
           FILe=SPIFFS.open("/SHOW/"+IMAGES[image_index], "r");
          if(!FILe) Serial.println("FILE NOT OPEN");
          size_t i = FILe.size();
            Serial.printf("File size is %u Bytes\n", i); 
          OpenlastTime=millis();
          Current_imageLine = 0;
           L=0;
           finish_chunk=true;
        }
        if(finish_chunk)
        {
          //Serial.print(F("chunck duration="));
           unsigned long  c=micros();
      FILe.read(_4kbuffer, 3888);
          /*Serial.println("read chunk");
         int i = 0; 
           while( (i < NUM_LEDS*3*9) && FILe.available())
          { 
               i += FILe.readBytes( ((char*)_4kbuffer) + i, (NUM_LEDS*3*9)-i);
            }*/
         //  Serial.printf("%u=%u\n", micros()-c,Current_imageLine);
         finish_chunk=false; 
        }
        if(/*Start_new_round && */(micros()-lastLineShow)> 100)
        {
          
          byte *p=&_4kbuffer[L*NUM_LEDS];
          memcpy(leds,p,NUM_LEDS);         
          
         // FastLED.show(); 
          lastLineShow=micros();
          
          Current_imageLine++;  
         // Serial.print(Current_imageLine);Serial.print(",");Serial.println(L);
          L++;
          if(L>=9) 
                {
                  L=0;
                  finish_chunk=true;
                }                                      
        }
       // if(image_index>2)             StartUpShow=false;
        //if(image_index>IMAGE_NUM)    image_index=3;
        //if( (micros()-lastLineShow)> lineInterval ) {read_line(Current_imageLine++,image_index); FastLED.show(); lastLineShow=micros(); }
      if(Current_imageLine >= IMAGES_LINES)   
      { 
        Serial.print("[frame]"); Serial.println((micros()-lastFrameShow));
        Current_imageLine = 0;
        lastFrameShow=micros(); 
        Start_new_round=false; 
        FILe.seek(0, SeekSet);
       // FILe=SPIFFS.open("/SHOW/"+IMAGES[image_index], "r");
       } 
         
      }

}

