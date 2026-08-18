//UDP-Listener
//
// for LRT and DMU 
// INKA-RESPATI
//
//  by Edes (5June2019)
//
// udp port = 1337
// send: ">ipconfig ?" to display ip setting
// send: ">ipconfig to=a.b.c.d ip=e.f.g.h gw=i.j.k.l nm=m.n.o.p;" 
//       to ip-addr or broadcast-addr to change ip setting in device with ip a.b.c.d
//

#include <EtherCard.h>
#include <IPAddress.h>
#include <EEPROM.h>
#include <avr/wdt.h>
  
#define SS_ETH 10
#define UDP_PORT 42

static byte default_IP[12]={192,168,28,42, 192,168,28,1, 255,255,0,0};
static byte MYdns[4]={8,8,8,8};

byte EEip[4],EEgw[4],EEnm[4];
byte MYip[4],MYgw[4],MYnm[4], MYmac[6];
byte NEW_value[12]; //new data received from server
byte EE_value[12];  //data stored in EEPROM
long int millis_5s;
long int millis_30s;
int Random1,Random2;
bool new_udp_data=false;
String last_received_udp;

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer

void udpSerialPrint(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, char *data, uint16_t len);
void display_setting();
void read_ip_from_eeprom();
void save_ip_to_eeprom();
void setting_ip(byte *data);
void delay_wdt(int milisecond);
void reset_wdt();
int parsing_update(String StringToParse);

//callback that prints received packets to the serial port

void setup(){
  Serial.begin(38400);
  
  wdt_disable();
  wdt_enable(WDTO_8S);
  
//  read_ip_from_eeprom();
  setting_ip(default_IP); 

  ether.udpServerListenOnPort(&udpSerialPrint, UDP_PORT);
  delay_wdt(1000);
  display_setting();
  delay_wdt(7500);
  Serial.println(F("$...*"));
  
  millis_5s = millis();
  millis_30s = millis();
}

void loop(){
  ether.packetLoop(ether.packetReceive());

  if (millis() >= millis_5s + 5000 ){
    millis_5s = millis();
    new_udp_data = false;
    wdt_reset();
  }
  if (millis() >= millis_30s + 30000 ){
    millis_30s = millis();
    if (!new_udp_data && last_received_udp.length() > 0){
      Serial.println(last_received_udp); // Mengirim pesan terakhir dengan Newline (\n)
    }
  }
}

// Fungsi pembantu untuk memecah string IP ("192.168.1.1") ke array byte
bool parseIPString(String ipStr, byte* targetArray, int startIndex) {
  int count = 0;
  int lastPos = 0;
  
  ipStr.trim();
  for (int i = 0; i <= ipStr.length(); i++) {
    if (i == ipStr.length() || ipStr.charAt(i) == '.') {
      String segment = ipStr.substring(lastPos, i);
      if (segment.length() == 0) return false;
      targetArray[startIndex + count] = (byte)segment.toInt();
      count++;
      lastPos = i + 1;
    }
  }
  return (count == 4);
}

int parsing_update (String StringToParse){
  String command, NEW_ip, NEW_gw, NEW_nm, TOip;
  int awal = 1, akhir; 

  wdt_reset();
  StringToParse.trim();
  
  if (!StringToParse.startsWith(">")) return 0;

  akhir = StringToParse.indexOf(' ', awal);
  command = StringToParse.substring(awal, akhir);
  command.toLowerCase();
  
  if (command != "ipconfig") {
    Serial.println(F("bukan ipconfig"));
    return(0);
  }
  
  awal = akhir + 1; 
  akhir = StringToParse.indexOf(' ', awal);
  TOip = StringToParse.substring(awal, akhir);
  TOip.trim();
  
  if (TOip == "?") return(2);
  
  awal = akhir + 1; 
  akhir = StringToParse.indexOf(' ', awal);
  NEW_ip = StringToParse.substring(awal, akhir);
  
  awal = akhir + 1; 
  akhir = StringToParse.indexOf(' ', awal);
  NEW_gw = StringToParse.substring(awal, akhir);
  
  awal = akhir + 1; 
  akhir = StringToParse.indexOf(';', awal);
  NEW_nm = StringToParse.substring(awal, akhir);

  byte targetIP[4];
  if (!parseIPString(TOip, targetIP, 0)) return 0;

  // Cek apakah perintah ini ditujukan ke IP modem/eth ini
  if ((targetIP[0] == ether.myip[0]) &&
      (targetIP[1] == ether.myip[1]) &&
      (targetIP[2] == ether.myip[2]) &&
      (targetIP[3] == ether.myip[3])) {

    // Parsing data IP baru, Gateway, dan Netmask ke array NEW_value
    if (parseIPString(NEW_ip, NEW_value, 0) &&
        parseIPString(NEW_gw, NEW_value, 4) &&
        parseIPString(NEW_nm, NEW_value, 8)) {
      return(1);
    }
  }
  else {
    Serial.println(F("bukan untuk IP ini"));
    return(0);
  }
  return(0);
}

void read_ip_from_eeprom(){
//  Serial.println("START read eeprom");
  wdt_reset(); //reset wdt

  // read EEPROM value
  for (int i=0;i<12;i++) EE_value[i] = EEPROM.read(i);

  // check if EEPROM value is empty (usually fresh burned)
  if ((EE_value[0]==0)&&(EE_value[4]==0)&&(EE_value[8]==0)){

    // if empty, update with default value
    for (int i=0;i<12;i++) {
      EEPROM.update(i,default_IP[i]);
      EE_value[i] = default_IP[i]; // so the content of EE-param is default
    }
  }
//  Serial.println("END read eeprom");
}

void save_ip_to_eeprom(){
//  Serial.println("Start Saving eeprom");
  wdt_reset(); //reset wdt
  for (int i=0;i<12;i++) EEPROM.update(i,NEW_value[i]);
//  Serial.println("END Saving eeprom");
}

void setting_ip(byte *data){
  wdt_reset();
  byte MYip[4] = {data[0], data[1], data[2], data[3]};
  byte MYgw[4] = {data[4], data[5], data[6], data[7]};
  byte MYnm[4] = {data[8], data[9], data[10], data[11]};
  byte MYmac[6] = {0xED, 0xE5, data[0], data[1], data[2], data[3]};
  
  if (ether.begin(sizeof Ethernet::buffer, MYmac, SS_ETH) == 0)
    Serial.println(F("Failed to access Eth controller"));
  else 
    Serial.println(F("Success access eth controller & setting MAC-addr"));
  
  ether.staticSetup(MYip, MYgw, MYdns, MYnm);
}

void reset_wdt(){
  wdt_disable();
  wdt_enable(WDTO_1S);
  while(1);
}

void delay_wdt(int milisecond){
  while (milisecond>1000){
    wdt_reset();
    delay(1000);
    milisecond -= 1000;
  }
  delay(milisecond);
}

void udpSerialPrint(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, char *data, uint16_t len){
  IPAddress src(src_ip[0], src_ip[1], src_ip[2], src_ip[3]);
  wdt_reset();
  new_udp_data = true;

  String string_udp = "";
  for(int i = 0; i < len; i++) string_udp += data[i];
  
  // Mengirim string UDP dengan baris baru (\n) agar diterima ESP32
  Serial.println(string_udp);

  last_received_udp = string_udp;
  int hasil = parsing_update(string_udp);
  
  if (data[0] == '$') {
    // Teks biasa untuk display LED Matrix
  }
  else if (data[0] == '>') {
    if (hasil == 1){
      Serial.println(F("$Change IP & wait Reset*"));
      save_ip_to_eeprom();
      reset_wdt();
    }
    else if (hasil == 2){
      display_setting();
    }
  }
}

void display_setting(){
  String setting_string = "$IP:";
  for (int i = 0; i < 4; i++){
    setting_string += ether.myip[i];
    if(i < 3) setting_string += ".";
  }
  setting_string += " GW:";
  for (int i = 0; i < 4; i++){
    setting_string += ether.gwip[i];
    if(i < 3) setting_string += ".";
  }
  setting_string += " NM:";
  for (int i = 0; i < 4; i++){
    setting_string += ether.netmask[i];
    if(i < 3) setting_string += ".";
  }
  setting_string += " MAC=";
  for (int i = 0; i < 6; i++){
    setting_string += ether.mymac[i];
    if(i < 5) setting_string += ":";
  }
  setting_string += "*";
  Serial.println(setting_string);
}
