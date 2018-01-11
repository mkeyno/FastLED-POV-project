
 
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h> 

#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>
#include <Hash.h>

#define HalSensor 2
#define NUM_LEDS   9
#define IMAGES_LINES 200
#define FASTLED_FORCE_SOFTWARE_SPI
#include <FastLED.h>
#define SPI_FLASH_SEC_SIZE 4096
#define HalSensor 14

#define DATA_PIN    13 // (8,15),( 4,2) (6,12) (7,13) 
#define CLOCK_PIN  14  //
CRGB leds[NUM_LEDS];

const char configHtml[] PROGMEM = R"=====(
<html>
<head>
<meta http-equiv="Content-Language" content="en-us">
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<title>Keyno Guidance & Control</title>
<script>
var MyIP=location.host;
var _Connection;
var link="ws://"+MyIP+":81";

window.onload = function ()
{
  setTimeout(ini_ws,1000);
 } 

function ini_ws()
{
//   alert(link);
_Connection = new WebSocket( link); 
//alert("status="+_Connection.readyState);
_Connection.onerror = function (error) {  
                    document.getElementById('_link').style.backgroundColor ="#FFA500";
                    alert('Conection Error '+'\n'+link);
                    }
_Connection.close = function (error) {  document.getElementById('_link').style.backgroundColor ="#FFE4E1";} //gray
_Connection.onopen = function (evt) {  
                  var newdate=new Date();
                  var C_date=pad2(newdate.getFullYear())+pad2(newdate.getDay()) +pad2(newdate.getDate()) ;
                  var C_sec=newdate.getSeconds()+ newdate.getMinutes()*60 + newdate.getHours()*3600; 
                  _Connection.send('SGT=' + C_date + C_sec); //15
                  document.getElementById('_link').style.backgroundColor ="#7FFF00"; // grenn
                  document.getElementById('current_data').value=newdate;
                  }
_Connection.onmessage = function(INCOME){parsing(INCOME.data); }   
  
}
function parsing(_income)
 {// alert(_income) 
  document.getElementById('_content').value=_income;
 }
function pad2(number) { return (number < 10 ? '0' : '') + number }
function createXHR(){
try{return new XMLHttpRequest();}catch (e){try {return new ActiveXObject("Microsoft.XMLHTTP");} 
catch (e){return new ActiveXObject("Msxml2.XMLHTTP");}}
}
function updatePath() {
 var command  = "UPT="+document.getElementById('_newfile').value;
 _Connection.send(command); }
function show_list_file() {
var _AJX=createXHR();    
_AJX.onreadystatechange = function(){ 
if(_AJX.readyState == 4)
{ var div = document.getElementById('showfiles');
div.innerHTML = _AJX.responseText;      // id of list box =listfile 
var _listBox=document.getElementById('listfile');
_listBox.selectedIndex=_listBox.options.length-1;                        
}}
_AJX.open("GET","/process?code=SLF" , true);
_AJX.send(null); 

}
function show_file_content(){
var patch= document.getElementById('_path').value ;
var file= document.getElementById('_file').value;  
var _AJX=createXHR();    
_AJX.onreadystatechange = function(){   
if(_AJX.readyState == 4){ 
var recieved =_AJX.responseText;
document.getElementById('_content').value=recieved; }}
_AJX.open("GET","/process?code=SFC&patch=" + patch+ '&file='+file, true);
_AJX.send(null); }
function update_file_content(){
var patch= document.getElementById('_path').value ;
var file= document.getElementById('_file').value;
var content=(document.getElementById('_content').value); 
_Connection.send("RUF=" + patch+ file+ "&" + content);
document.getElementById('_content').value="";}
function removefile(){
var path=document.getElementById("_path").value;
var file=document.getElementById("_file").value;
var _AJX=createXHR();    
_AJX.onreadystatechange = function(){   
if(_AJX.readyState == 4){  
var recieved =_AJX.responseText; show_list_file();            
}}
_AJX.open("GET","/process?code=RRF&patch=" + path+ '&file='+file, true);
_AJX.send(null);}
function CreateFile(){
var path=document.getElementById("_path").value;
var file=document.getElementById("_file").value;
var _AJX=createXHR();    
_AJX.onreadystatechange = function(){   
if(_AJX.readyState == 4){  
var recieved =_AJX.responseText;  show_list_file();}}
_AJX.open("GET","/process?code=CRF&patch=" + path+ '&file='+file, true);
_AJX.send(null);}
function renamefile(){
var path=document.getElementById("_path").value;
var oldfile=document.getElementById("_file").value;
var newfile=document.getElementById("_newfile").value;
var _AJX=createXHR();    
_AJX.onreadystatechange = function(){   
if(_AJX.readyState == 4){  
var recieved =_AJX.responseText;  show_list_file();}}
_AJX.open("GET","/process?code=RNF&patch=" + path+ '&oldfile='+oldfile + '&newfile='+newfile, true);
_AJX.send(null); }
function parse_line()
{
var e = document.getElementById("listfile");
var line = e.options[e.selectedIndex].text;
var myArray=line.split("/");
var num=myArray.length; 
if(num>2){ 
document.getElementById("_path").value='/'+myArray[1]; 
document.getElementById("_file").value='/'+myArray[2]; }
else {
document.getElementById("_path").value='/'; 
document.getElementById("_file").value=myArray[1];}
}
</script>
<script type="text/javascript">      
//window.onload = function (){setTimeout(Request_IP,1000);} 
</script>
</head>
<body>
<input type="button" id="Bu51on1" onclick="upload_file();return false;" name="" value="Upload file" style="position:absolute;left:809px;top:3px;width:73px;height:30px;z-index:0;">
<script>
function upload_file()
{
var file=document.getElementById("FileUpload1");
var path=document.getElementById("_path").value + document.getElementById("path").value;
if(file.files.length === 0)  return;   
var formData = new FormData();

   formData.append("data", file.files[0], path); 
var _AJX = new XMLHttpRequest(); 
_AJX.onreadystatechange = function(){
if (_AJX.readyState == 4){ 
if(_AJX.status != 200)    alert("ERROR["+_AJX.status+"]: "+_AJX.responseText); 
else                     show_list_file(); 
Show_Image_List();}}; 
_AJX.open("POST", "/upload");
_AJX.send(formData); }
</script>
<input type="file" id="FileUpload1" style="position:absolute;left:714px;top:5px;width:73px;height:23px;line-height:23px;z-index:1;" name="FileUpload1" multiple ="false" onchange="check_validity();return false;"> 
<script>
function check_validity()
 {
 var file=document.getElementById("FileUpload1");
 var path=document.getElementById("path");
  if(file.files.length === 0)     return; 
  var filename = file.files[0].name; 
  var ext = /(?:\.([^.]+))?$/.exec(filename)[1]; 
  var name = /(.*)\.[^.]+$/.exec(filename)[1]; 
  if(typeof name !== undefined){ filename = name; } 
  if(typeof ext !== undefined){ 
       if(ext === "html") ext = "htm"; 
  else if(ext === "jpeg") ext = "jpg"; 
  filename = filename + "." + ext; }
  if(path.value === "/" || path.value.lastIndexOf("/") === 0)  { path.value = "/"+filename; } 
  else { path.value = path.value.substring(0, path.value.lastIndexOf("/")+1)+filename; }
 }  
</script>
<input type="text" id="path" style="position:absolute;left:552px;top:6px;width:145px;height:23px;line-height:23px;z-index:2;" name="Editbox1" value="">
<div id="showfiles" style="position:absolute;left:552px;top:152px;width:110px;height:71px;z-index:3">
</div>
<input type="button" id="Button3" onclick="show_list_file();return false;" name="" value="show file List" style="position:absolute;left:802px;top:118px;width:85px;height:25px;z-index:4;">
<textarea name="TextArea1" id="_content" style="position:absolute;left:4px;top:42px;width:532px;height:387px;z-index:5;" rows="23" cols="86"></textarea>
<input type="button" id="Button5" onclick="update_file_content();return false;" name="" value="Update File content" style="position:absolute;left:132px;top:6px;width:127px;height:25px;z-index:6;">
<input type="button" id="Button2" onclick="show_file_content();return false;" name="" value="Show file content" style="position:absolute;left:4px;top:6px;width:119px;height:25px;z-index:7;">
<input type="text" id="_file" style="position:absolute;left:435px;top:8px;width:92px;height:23px;line-height:23px;z-index:8;" name="Editbox1" value="" text-align="right">
<input type="text" id="_path" style="position:absolute;left:318px;top:8px;width:82px;height:23px;line-height:23px;z-index:9;" name="Editbox1" value="" text-align="right">
<div id="wb_Text3" style="position:absolute;left:268px;top:12px;width:50px;height:18px;z-index:10;text-align:left;">
<span style="color:#000000;font-family:Arial;font-size:16px;">patch</span></div>
<div id="wb_Text4" style="position:absolute;left:405px;top:12px;width:30px;height:18px;z-index:11;text-align:left;">
<span style="color:#000000;font-family:Arial;font-size:16px;">file</span></div>
<input type="button" id="Button1" onclick="removefile();return false;" name="" value="Remove file" style="position:absolute;left:552px;top:80px;width:80px;height:25px;z-index:12;">
<input type="button" id="Button7" onclick="updatePath();return false;" name="" value="Update Path" style="position:absolute;left:552px;top:118px;width:80px;height:25px;z-index:13;">
<input type="button" id="Button4" onclick="renamefile();return false;" name="" value="Rename file" style="position:absolute;left:679px;top:80px;width:84px;height:25px;z-index:14;">
<input type="button" id="Button6" onclick="CreateFile();return false;" name="" value="Create  file" style="position:absolute;left:802px;top:80px;width:80px;height:25px;z-index:15;">
<input type="text" id="_newfile" style="position:absolute;left:679px;top:118px;width:82px;height:23px;line-height:23px;z-index:16;" name="Editbox1" value="" text-align="right">
<input type="submit" id="_link" onclick="ini_ws();return false;" name="" value="link" style="position:absolute;left:552px;top:42px;width:330px;height:24px;z-index:17;">
</body>
</html>
)=====";

String SSID="keyno"; //add 15-29
String SSIDPASS="09127163464";//add 30-44
String AP_SSID="Smart POV";//add 45-59
String AP_SSIDPASS="12345678";//add 60-74 
bool ACCESS=false,STATIC=false; 
unsigned long Ego_Seconds=27000;
byte client_num=0;

String MyMode=""; // add 0
String myHostname = ""; //add 1-14

byte reset_number=0;//add 76
String Server_IP="192.168."; // add 77-79
String Static_IP="192.168.";//81-83
String MyIP="";
String PerfixID="";
String license="Keyno";
String Software="KEASI Ver 0.7";
String uploadError="";  
String PATH="/";
String preSetUser="";
String preSetPass="";
String STRING="",Last_store_time="",Last_store_date="";


bool StartUpShow=true,SHOW=false,TEST=false;
String IMAGES[30]={};
byte  IMAGES_Prior[]={0,1,2,3,4};
byte IMAGE_NUM=0;
int DURATION[]={5,5,5,5,5};

static int image_index=0, Current_imageLine=0;
 
static  volatile uint32_t t_start_round=0,t_stop_round=0,lastFrameShow=0;
static  volatile uint32_t lineInterval=100,RPM=0;
static  volatile bool Start_new_round=false;
uint32_t free_Space_location_start;
uint32_t start_address_of_imagefile[10];
static uint32_t current_raw_location;
//uint8_t * BUFFER;
uint8_t  * LED_BUFFER;
byte number_of_file;     
static uint32_t _memory_pointer=0,OpenlastTime=0,frame_time=0,lastLineShow=0;

WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer server(80);

IPAddress apIP(192, 168, 4, 1),_ip;

File fsUploadFile;

void UpdateContent(String path, String income){  // general file   
        File f = SPIFFS.open(path, "w+");   
  if (!f) {
    Serial.println("[F-w-"+path+"]");// now write two lines in key/value style with  end-of-line characters  
    return;
      }   
  else {  // income.replace(";", "\n\r"); 
  f.println(income); f.close();  }       
    
}
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
void Restore_address(void){
  String s=read_line("/data/address.inf",0);
  byte As=0;
  for(byte i=0;i<IMAGE_NUM;i++)
  {
    As=s.indexOf('@');
    start_address_of_imagefile[i]=s.substring(0,As).toInt();
    Serial.printf("Address of File %s=%u\n",IMAGES[i].c_str(),start_address_of_imagefile[i]);
    s=s.substring(As+1);
  }
}
void  Store_address(void){
  String s="";
  for(byte i=0;i<IMAGE_NUM;i++) s+=String(start_address_of_imagefile[i])+"@";
  Serial.println(s);
  UpdateContent("/data/address.inf",s);  
}
String compose(void){
//BYTES=compose_bytes();  
String s="";
s=myHostname+"@"+SSID+"@"+SSIDPASS+"@"+AP_SSID+"@"+AP_SSIDPASS+"@"+Static_IP+"@"+Server_IP+"@";//+BYTES+"@";
return s;
}
void decompose(String s){
//A@myHostname@SSID@
byte As=0;

As=s.indexOf('@'); 
As=s.indexOf('@'); myHostname=s.substring(0,As); if(myHostname.length()<3) myHostname="KGC";     s=s.substring(As+1);
As=s.indexOf('@');       SSID=s.substring(0,As); if(SSID.length()<3)             SSID="Unknown"; s=s.substring(As+1);
As=s.indexOf('@');   SSIDPASS=s.substring(0,As); if(SSIDPASS.length()<3)     SSIDPASS="Unknown"; s=s.substring(As+1);
As=s.indexOf('@');    AP_SSID=s.substring(0,As); if(AP_SSID.length()<3)       AP_SSID="Smart_Node_" +String(ESP.getChipId(), HEX);   s=s.substring(As+1);
As=s.indexOf('@');AP_SSIDPASS=s.substring(0,As); if(AP_SSIDPASS.length()<3)     AP_SSIDPASS="12345678";   s=s.substring(As+1);

As=s.indexOf('@'); Static_IP=s.substring(0,As); if(Static_IP.length()<3) Static_IP="192.168.1.11"; s=s.substring(As+1);
As=s.indexOf('@'); Server_IP=s.substring(0,As); if(Server_IP.length()<3) Server_IP="192.168.4.1"; 

}
 void restore_setting(void) { 
 STRING=read_line("/data/mydevice.dat",0);
 decompose(STRING);
 
 preSetUser=String(ESP.getChipId());//
preSetPass=String(ESP.getChipId(), HEX);
preSetPass.toUpperCase();
}
void store_setting(void){
STRING=compose(); 
UpdateContent("/data/device.inf",STRING);  
Serial.println("[Stored]");
}
String Retrive_file(String _File,String perfix){
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
void copy_file_from_SPIFF_to_Raw_Flash(String path, byte file_num){
uint32_t startTime = millis();
 File f = SPIFFS.open(path, "r");
 if(!f) { Serial.println("Could not open file for reading"); return; } 
size_t i = f.size();
start_address_of_imagefile[file_num]=current_raw_location;
 Serial.printf("File size is %u Bytes Start location =%u\n", i,start_address_of_imagefile[file_num]);
  uint8_t* BUFFER = new uint8_t[SPI_FLASH_SEC_SIZE];
 while(i > SPI_FLASH_SEC_SIZE){
                 f.read(BUFFER, SPI_FLASH_SEC_SIZE);
                 optimistic_yield(10000); 
                 ESP.flashEraseSector(current_raw_location/FLASH_SECTOR_SIZE);
                 ESP.flashWrite(current_raw_location,(uint32_t *) BUFFER, SPI_FLASH_SEC_SIZE);      
                 Serial.print("."); i -= SPI_FLASH_SEC_SIZE;
                 current_raw_location+=SPI_FLASH_SEC_SIZE;
                 } 
 f.read(BUFFER, i); 
 for(int n=0;n<i;n++){Serial.print((char)BUFFER[n]); Serial.print(",");}Serial.println();
 optimistic_yield(10000); 
 ESP.flashEraseSector(current_raw_location/FLASH_SECTOR_SIZE);
 ESP.flashWrite(current_raw_location,(uint32_t *) BUFFER, i);   
 current_raw_location+=i;  
 f.close();
 delete [] BUFFER; 
 Serial.printf("copy took %u ms, memory  end address=%u\n",  millis() - startTime,current_raw_location);  
}
/////////////////////////////////              webSocket              ///////////////////////////////////////
void parse_webSocket(String income, byte num){
     byte eqI1 = income.indexOf('=');
 String cammand = income.substring(0, eqI1);// Serial.print(" cammand=");Serial.println(cammand);
 String value   = income.substring(eqI1+1); //Serial.print(" value=");Serial.println(value);
     if(cammand=="RST")  { // [-Standalone-]
              /*
              wifimode
              hostID
              localip
              serverip
              _SSID
              AP_name
              software
              license
              
              EactuaterID
              EactuaterIP
              EsensorID
              Esensordescrip
              */
              String s="";
              if(MyMode=="A") s="Access Point"; 
              else            s="Station";
             // Serial.println("$D"+IDs[0]+"$"+DESCRIPT[0]+"$D"+IDs[1]+"$"+DESCRIPT[1]+"$D"+IDs[2]+"$"+DESCRIPT[2]);
             webSocket.sendTXT(num, "RST$"+
                                          s           +"$"+
                            myHostname  +"$"+
                            MyIP        +"$"+
                            Server_IP   +"$"+
                            SSID        +"$"+
                            AP_SSID     +"$"+
                            Software    +"$"+
                            license     +"$"
                               
                            );
            } 
//'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
else if(cammand=="WFM")  { lineInterval+=100;  webSocket.sendTXT(client_num,"RPM$"+String(lineInterval));}//EEPROM.write(1, value[0]);EEPROM.commit(); } // Set Wifi Mode MyMode  [-Standalone-]
else if(cammand=="SHN") {lineInterval-=10;  webSocket.sendTXT(client_num,"RPM$"+String(lineInterval));}// myHostname=value; // Set Host Name myHostname   [-Standalone-]
else if(cammand=="SNW") {  // SSID SSIDPASS Select Network user&pass        [-Standalone-]     
                   eqI1 = value.indexOf('&');
             SSID= value.substring(0, eqI1);// Serial.print(" SSID=");Serial.println(_ssid);
             SSIDPASS= value.substring(eqI1+1); //Serial.print(" pass=");Serial.println(pass);  Serial.println(lineInterval);              
            }
else if(cammand=="APN")  {lineInterval+=1;  webSocket.sendTXT(client_num,"RPM$"+String(lineInterval));}//AP_SSID=value; // Set AP Name AP_SSID   [-Standalone-]
else if(cammand=="APP") {lineInterval-=1;  webSocket.sendTXT(client_num,"RPM$"+String(lineInterval));}// AP_SSIDPASS=value; // Set AP Password   AP_SSIDPASS      [-Standalone-]
else if(cammand=="ESI") { EEPROM.write(2, value[0]);EEPROM.commit();(value[0]=='1')?STATIC=true:STATIC=false;} //  [-Standalone-]                          
else if(cammand=="SIP") {lineInterval-=100;  webSocket.sendTXT(client_num,"RPM$"+String(lineInterval));} //Server_IP=value;// Set Server IP Server_IP     [-Standalone-]             
                
else if(cammand=="STI") {lineInterval+=10;  webSocket.sendTXT(client_num,"RPM$"+String(lineInterval));}// Static_IP=value;// Set Static_IP  81-82   [-Standalone-]
 
else if(cammand=="SGT")  { //time of browser webSocket recieved: SGT=20160510 77666  119-133   [---SN---]
               Last_store_date=value.substring(2,8);  // 
               Last_store_time=value.substring(8);                           
                          Ego_Seconds=Last_store_time.toInt();   Serial.print(" Base second=");Serial.println(Ego_Seconds);   //77666000   
                          } // Set global date setting   
 
else if(cammand=="SVE")  {if(TEST) TEST=false; else TEST=true;Serial.println(TEST);}//webSocket.sendTXT(num,"SVE$");store_setting(); }

//''''''''''''''''''''''''''''''''''''' setting page ''''''''''''''''''''''''''''''''''''''''''''''
else if(cammand=="SHO") {  //SHO=1       [-Standalone-]
                          (value[0]=='1')?SHOW=true:SHOW=false;
                          if(SHOW) _memory_pointer=start_address_of_imagefile[image_index];
                         Serial.print("SHOW=");Serial.println(SHOW);
                          }  
 
else if(cammand=="CLR")  ;//// clear store sensor data   [-Standalone-]
else if(cammand=="SIM")   //  SIM=0#1@1 value.indexOf('#');
                      {
                        byte _index = value.substring(0, value.indexOf('#')).toInt(); // Serial.print(" _index");Serial.println(_index);
                        byte Priority=value.substring(value.indexOf('#')+1, value.indexOf('@')).toInt();//  Serial.print(" Priority=");Serial.println(Priority);
                        byte duration=value.substring(value.indexOf('@')+1).toInt(); // Serial.print(" duration=");Serial.println(duration);
                        EEPROM.write(_index+20, Priority);  EEPROM.commit();
                        EEPROM.write(_index+70, duration);  EEPROM.commit();             
                      } 

//'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''             
else if(cammand=="RRE")    ESP.restart();// restart    [----SN---]
else if(cammand=="CSP") if(IMAGE_NUM) {  for(byte i=0;i<IMAGE_NUM;i++) copy_file_from_SPIFF_to_Raw_Flash("/SHOW/"+IMAGES[i], i);   Store_address(); } //  [-Standalone-]
else if(cammand=="RUF")  { // UpdateContent                                      [-CONFIG-]       
             eqI1 = value.indexOf('&');
             String filename = value.substring(0, eqI1); //Serial.print(" filename=");Serial.println(filename);
             String content  = value.substring(eqI1+1); //Serial.print(" content=");Serial.println(content);
             UpdateContent(filename,content);      
            }     
else if(cammand=="RUP")   PATH=value;// EEPROM_Write(value,0,1); // Set AP IP     [-CONFIG-] 
}
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
 client_num=num;
 switch(type) {
        case WStype_DISCONNECTED:
                                 Serial.printf("[WS:%u Disconnected!]\n", num); 
                 ACCESS=false;
        break;
        case WStype_CONNECTED:   { 
                       IPAddress ip = webSocket.remoteIP(num);
                   ACCESS=true;
                  Serial.print("[WS:]Clinet "); Serial.print(num); Serial.print(" CONNECTED with IP: ");Serial.println(ip);
                  
                 } 
        break;
        case WStype_TEXT:    // this for  webSocket request          
                String text = String((char *) &payload[0]);
                Serial.print("[WS:"); Serial.print(num);Serial.print("]");Serial.println(text);
                parse_webSocket(text,num);
        break;
        
        
      }

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
void show_user_list(String path,String _ID,String parseFunction){ 
                     //"/data/user.dat","ListUserListBox","parse_line_and_put_inform"
  String records="<select name=\"Combobox2\" size=\"5\" id=\""+_ID+"\"  onChange=\""+parseFunction+"();\"   >";
  File f = SPIFFS.open(path, "a+");   ///a+ if not exist then create it 
  if (!f) return server.send(200, "text/plain", records); 
  byte line_num_count=0;
  //String line="";
   while(f.available()) { 
                         String line = f.readStringUntil('\n');
                           
                if(line.length() >2) {//Serial.print("line number:");Serial.print(line_num_count);Serial.println("="+line);
                           records+="<option value=\"" + String(line_num_count)+"\">" +line +"</option>";
                          line_num_count++; }
               
              } 
   f.close(); 
  
 records+="</select>";
 server.send(200, "text/plain", records); 
}
void AddUser(String user,String pass,String scop){
  String line="\n"+user+ "," + pass + "," + scop;
  //Serial.println(line); 
   File f = SPIFFS.open("/data/user.dat", "a+");  
  if (!f) return ; 
  f.print(line);
  f.close();
}
void RemoveUser(byte num){
  //Serial.print("lndex number:");Serial.println(num);
  String temp="";
  File f = SPIFFS.open("/data/user.dat", "r");
  byte num_count=0;
  while(f.available()) {//Lets read line by line from the file      
              String line = f.readStringUntil('\n');
             if(line.length() >2) {
                       //Serial.print("line number:");Serial.print(num_count);Serial.println("="+line);
                      if(num!=num_count)temp=temp+line+"\n\r";              
                      num_count++;
                       }
            } 
  
  f.close();
  f = SPIFFS.open("/data/user.dat", "w+");
  f.println(temp);
   f.close();
}
void Createfile(String path, String filename){
  if(SPIFFS.exists(path+filename))  return server.send(500, "text/plain", "FILE EXISTS"); 
  File file = SPIFFS.open(path+filename, "w");
    if(file) file.close();   
  else                         return server.send(500, "text/plain", "CREATE FAILED");
}
void removefile(String path, String filename){
  if(!SPIFFS.exists(path+filename))  return server.send(404, "text/plain", "FileNotFound");  
  SPIFFS.remove(path+filename);             server.send(200, "text/plain", "removed");
}
void RenameFile(String patch,String oldfilename,String newfilename){
 String fold=patch+oldfilename;
 String fnew=patch+newfilename;
if (SPIFFS.exists(fold))
          {
            SPIFFS.rename(fold,fnew);
            server.send(200, "text/plain", "Renamed");
          }
  
}
void show_listfile(void){ ///replace all item of list box
   String records="<select name=\"Combobox1\" size=\"20\" id=\"listfile\"  onChange=\"parse_line();\"   >";
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
void show_file_content(String patch, String filename){ //show_file_content() _AJX RGP
  String content="";   
  File f = SPIFFS.open(patch+filename, "r");
  while(f.available()) {//Lets read line by line from the file      
              String line = f.readStringUntil('\n');
              
              if(line.length() >2) 
              { //Serial.print("line number:");Serial.print(line_num_count);Serial.println("="+line);
              content+=(line+"\n");
             
              }
            } 
  f.close();
  server.send(200, "text/plain", content);
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
void handleFileUpload(){ // "/update"
  if(server.uri() != "/upload" ) return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
                      String filename = upload.filename;                            
                       if(!filename.startsWith("/")) filename = "/"+filename; 
                       if(PATH.length()>3) filename=PATH+filename;
                       
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
byte list_images_filename(String _path){
    Dir dir = SPIFFS.openDir(_path);
    int j=0;
    while (dir.next()) IMAGES[j++] = dir.fileName().substring(6);    
    for(int i=0;i<j;i++) Serial.printf("Image File=%s\n",IMAGES[i].c_str());
  return j;    
}
void sort_image(void){
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
void showActiveImage(void){         
  IMAGE_NUM=list_images_filename("/SHOW");
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
void Show_Image_List(void){
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
void update_show_list(void){
  IMAGE_NUM=list_images_filename("/SHOW");
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
 ////////////////////////////////////////////////////////////////
void process() // this for httml request
{
  Serial.print("[HTML]"); Serial.print(server.arg(0)); Serial.print("  ACCESS="); Serial.println(ACCESS);    
       if(server.arg(0)=="IMG") Show_Image_List() ;// CONFIG  
  else if(server.arg(0)=="SLF") show_listfile() ;// CONFIG  
  else if(server.arg(0)=="RNF") RenameFile(server.arg(1),server.arg(2),server.arg(3));//http://192.168.4.1/process?code=RRN&patch=patch& [-CONFIG-]
  else if(server.arg(0)=="CRF") Createfile(server.arg(1),server.arg(2)) ; //http://192.168.4.1/process?code=RCF&patch=patch&file=file   [-CONFIG-]
  else if(server.arg(0)=="RRF") removefile(server.arg(1),server.arg(2)) ; //http://192.168.4.1/process?code=RRF&patch=patch&file=file [-CONFIG-]
  else if(server.arg(0)=="SFC") show_file_content(server.arg(1),server.arg(2)) ;//http://192.168.4.1/process?code=RGP&patch=patch&file=file [-CONFIG-] 
  else if(server.arg(0)=="USL") update_show_list(); //update_show_list()->_AJX-> _imageList  [-CONFIG-]
  else if(server.arg(0)=="RUL") show_user_list("/data/user.dat","ListUserListBox","parse_line_and_put_inform"); 
  else if(server.arg(0)=="AUS") AddUser(server.arg(1),server.arg(2),server.arg(3));//http://192.168.4.1/process?code=RAU&user=user&pass=pass&scop=scop [-Standalone-]
  else if(server.arg(0)=="RUS") RemoveUser(server.arg(1)[0]-'0');//http://192.168.4.1/process?code=RRU&index=index  [-Standalone-]
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
void handleRoot() {  handleFileRead("/Sindex.html");   }
void send_config_html(void){server.send ( 200, "text/html", configHtml );  }
void handleConfig(){  if(!server.authenticate(preSetUser.c_str(), preSetPass.c_str())) return server.requestAuthentication();
 ACCESS=true;
 send_config_html();}
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
void print_chip_info(void){
   preSetUser=String(ESP.getChipId());//
 preSetPass=String(ESP.getChipId(), HEX);
 preSetPass.toUpperCase();
 Serial.println("\n\nDevice ID=" + preSetPass +"(" + preSetUser + ")");
Serial.print("Slected FlashChipSize: ");Serial.println(formatBytes(ESP.getFlashChipSize()));
 Serial.print("FlashChipRealSize: ");Serial.println(formatBytes(ESP.getFlashChipRealSize()));
Serial.print("SketchSize:");Serial.println(formatBytes(ESP.getSketchSize()));
Serial.print("FreeSketchSpace:");Serial.println(formatBytes(ESP.getFreeSketchSpace()));
Serial.print("FreeHeap:");Serial.println(formatBytes(ESP.getFreeHeap()));

//Serial.print("\nChipId: ");Serial.println(ESP.getChipId());
Serial.print("CpuFreqMHz: ");Serial.println(ESP.getCpuFreqMHz());  
Serial.print("SdkVersion: ");Serial.println(ESP.getSdkVersion());
Serial.print("FlashChipSpeed: ");Serial.println(ESP.getFlashChipSpeed());
Serial.print("FlashChipMode: ");Serial.println(ESP.getFlashChipMode()); 
}
void Rotate_1_round(void){  
  t_stop_round=micros();
  RPM=(t_stop_round-t_start_round)/(IMAGES_LINES);
  lineInterval=(lineInterval+RPM)/2;  
  t_start_round=t_stop_round;
  Start_new_round=true; 
}


static uint8_t tint_poz = 0;

void setup() {
 Serial.begin(115200);
// Serial.begin(115200, SERIAL_8N1,SERIAL_TX_ONLY);
  delay(500);
    Serial.setDebugOutput(true);
  Serial.println(F("\n \nStarting.....")); 

 SPIFFS.begin();
 EEPROM.begin(128);
 FastLED.addLeds<APA102,DATA_PIN,CLOCK_PIN,RGB,DATA_RATE_MHZ(16)>(leds,NUM_LEDS);
   FastLED.setBrightness(255);
  FastLED.clear();
 FastLED.show(); 
 print_chip_info();
 
 DIR(); 
 IMAGE_NUM=list_images_filename("/SHOW");
 sort_image();
Restore_address();
WiFi.mode(WIFI_AP);
WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
WiFi.softAP(AP_SSID.c_str(),AP_SSIDPASS.c_str());                                
delay(500); // Without delay I've seen the IP address blank

//MyIP=WiFi.softAPIP().toString();
Serial.print("AP IP address: ");Serial.println(WiFi.softAPIP());
 server.on("/upload",      HTTP_POST, [](){ Serial.println("Upload post");    server.send(200, "text/plain", ""); /* Redirect("/setting.html");*/  }, handleFileUpload);
   server.on("/updateImage", HTTP_POST, [](){ Serial.println("Upload post");    server.send(200, "text/plain", "Loaded"); /* Redirect("/setting.html");*/  }, handleFileUploadImage);
  server.on("/", handleRoot);
   server.on("/process", process); 
  server.on("/config.html", handleConfig);
  server.onNotFound(handleOther);
  
  server.begin();  Serial.println(F("\t\t\tHTTP server started"));  
  
  const char * headerkeys[] = {"User-Agent","Cookie"} ; //here the list of headers to be recorded
  size_t hz = sizeof(headerkeys)/sizeof(char*); 
  server.collectHeaders(headerkeys, hz ); //ask server to track these headers
  
     current_raw_location=(ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
   Serial.printf("New Start Address= %u\n",current_raw_location);
 LED_BUFFER = (uint8_t *)malloc(NUM_LEDS*3); 
 Serial.print(F("Number of Image=")); Serial.println(IMAGE_NUM);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
 delay(500);
 pinMode(HalSensor, INPUT);
 attachInterrupt(digitalPinToInterrupt(HalSensor), Rotate_1_round, FALLING);
 OpenlastTime=millis();
}

byte hue=0;

void loop() {
 if(!SHOW && !TEST) server.handleClient();   
  if(SHOW)
		  {
			if( (millis()- OpenlastTime) >DURATION[image_index]*1000)	{      
						 if(image_index>=IMAGE_NUM) image_index=0; 
						 _memory_pointer=start_address_of_imagefile[image_index];
						 Serial.printf("File number=%u name:%s address:%u duration:%u\n",
						 image_index,IMAGES[image_index].c_str(),start_address_of_imagefile[image_index],DURATION[image_index]);
						 Current_imageLine=0;     
						 image_index++; 
						 OpenlastTime=millis();
						}
			 if((micros()-lastLineShow)> lineInterval)
													 {  
													  lastLineShow=micros();
														  ESP.flashRead(_memory_pointer,(uint32_t *)leds    ,NUM_LEDS*3 );  
														  FastLED.show();
														   _memory_pointer+=(NUM_LEDS*3);
															Current_imageLine++;
														   delay(LineIntervalDelay);   
													  }
			if(Current_imageLine>=IMAGES_LINES)	{
												Current_imageLine=0;
												 _memory_pointer=start_address_of_imagefile[image_index-1]; 
												}
				 
		} 
optimistic_yield(1000);   
}
