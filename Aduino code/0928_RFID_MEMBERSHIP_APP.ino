#include <SoftwareSerial.h>
SoftwareSerial RFID(D1, D0); // RX and TX

#include <ESP8266WiFi.h>                                                    // esp8266 library
#include <FirebaseArduino.h>                                                // firebase library 
  

// PROJECT : 0908 RFID MEMBERSHIP FIREBASE  
#define FIREBASE_HOST "rfid-membership-firebase.firebaseio.com"                         // the project name address from firebase id
#define FIREBASE_AUTH "7eebi2NUxy9KEFNr5Q19uv2QhsgJlPD1bwpH0ovD"                    // the secret key generated from firebase
 
#define WIFI_SSID "tatatata"                                          // input your home or public wifi name 
#define WIFI_PASSWORD "qwer1234"                                    //password of wifi ssid 
   
int data1 = 0;
int ok = -1;

#define ACCEPT D2
#define REJECT D3  

// use first sketch in http://wp.me/p3LK05-3Gk to get your tag numbers
#define TOTAL_TAG  5 
int tag1[14] = {2,54,67,48,48,55,69,70,50,57,57,55,57,3}; 
int tag2[14] = {2,54,67,48,48,55,69,70,50,48,57,69,57,3}; 
int tag3[14] = {2,54,67,48,48,55,69,70,50,49,53,70,53,3};  
int tag4[14] = {2,54,67,48,48,55,69,70,50,51,65,68,65,3}; 
int tag5[14] = {2,54,66,48,48,68,70,53,51,49,53,70,50,3}; 

int newtag[14] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // used for read comparisons
int temptag[14];

String tempStr;

boolean WIFI_SET(){

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                     //try to connect with wifi
    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID);
    
     while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP Address is : ");
    Serial.println(WiFi.localIP());                                            //print local IP address
    
}
   
String comfirmedTag;

void setup()
{
    RFID.begin(9600);    // start serial to RFID reader
    Serial.begin(9600);  // start serial to PC 
    Serial.println("begin");  
    
    pinMode(LED_BUILTIN, OUTPUT);                                         //Start reading dht sensor
    digitalWrite(LED_BUILTIN, HIGH);
    
    WIFI_SET();  
    
    FIREBASE_SET();
  
    digitalWrite(LED_BUILTIN, LOW);         // OFF
    
    pinMode(ACCEPT, OUTPUT);              // for status LEDs
    pinMode(REJECT, OUTPUT);

    digitalWrite(ACCEPT,LOW);               // OFF
    digitalWrite(REJECT,LOW);               // OFF
}
 
bool Read_state;

bool readTags()
{
  ok = -1;
  int tag_no;
  if(Read_state == false)
  {
      if (RFID.available() > 0) 
      {
        // read tag numbers
        delay(100); // needed to allow time for the data to come in from the serial buffer.
    
        for (int i = 0 ; i < 14 ; i++) // read the rest of the tag
        {
          data1 = RFID.read();
          temptag[i] = newtag[i] = data1;
          
          Serial.print(newtag[i]);
          if(i<13)
            Serial.print(",");
        }
        Serial.println("");
        RFID.flush(); // stops multiple reads

        // do the tags match up?
        tag_no = checkmytags();
        Read_state = true; 
      } 
  }
  else
  { 
      while(RFID.available()) RFID.read();  // EMPTY RX BUFFE  
      if (!RFID.available()) 
      {
        Read_state = false; 
        //Serial.println("Released");
      }
  }

  // now do something based on tag type
  if (ok > 0) // if we had a match
  {
    Serial.print("Accepted NO:"); Serial.println(tag_no); 
    Serial.println("");
    comfirmedTag="";
    for(int i =0; i < 14; i++)
    {
       comfirmedTag += newtag[i];
    }

    //Firebase.setString("TAG_READ",comfirmedTag);
    
    digitalWrite(ACCEPT, HIGH);           // ON
    delay(2000);
    digitalWrite(ACCEPT, LOW);            // OFF
    ok = -1;
    return true;
  } 
    
  else if (ok == 0) // if we didn't have a match
  {
    Serial.println("Rejected");
   Serial.println("");
    digitalWrite(REJECT, HIGH);
    delay(2000);
    digitalWrite(REJECT, LOW);  
    ok = -1;
  }
  return false;
}

boolean comparetag(int NEW_TAG[14], int OLD_TAG[14])
{
  boolean result = false;
  int cnt = 0;
  for (int i = 0 ; i < 14 ; i++)
  {
    if (NEW_TAG[i] == OLD_TAG[i])
    {
      cnt++;
    }
  }
  if (cnt == 14)
  {
    result = true;
  }
  return result;
}

int tag;
int checkmytags() // compares each tag against the tag just read
{
  ok = 0; // this variable helps decision-making,
  // if it is 1 we have a match, zero is a read but no match,
  // -1 is no read attempt made
   // if (comparetag(newtag, tag1) == true & tag != 1)
  if (comparetag(newtag, tag1) == true )
  {
    ok++;
    tag = 1;
    //Serial.println("111");
  }
   //  if (comparetag(newtag, tag2) == true & tag != 2)
     if (comparetag(newtag, tag2) == true )
  {
    ok++;
    tag = 2;
    //Serial.println("222");
  }
   //  if (comparetag(newtag, tag3) == true & tag != 3)
    if (comparetag(newtag, tag3) == true)
  {
    ok++;
    tag = 3;
    //Serial.println("333");
  }
   //  if (comparetag(newtag, tag4) == true & tag != 4)
     if (comparetag(newtag, tag4) == true)
  {
    ok++;
    tag = 4;
    //Serial.println("444");
  }
   //if (comparetag(newtag, tag5) == true & tag != 5)
     if (comparetag(newtag, tag5) == true)
  {
    ok++;
    tag = 5;
    //Serial.println("555");
  }
  return tag;
}

char buffer[20];
int bufferIndex;  
int DB_GET_CNT,DB_GET_CNT_STORED;
bool MODE_STATE;
String MODE_GET,Email_GET,TAG_GET;

void READ_AND_FIRE()
{ 
   if(readTags())
   { 
      
      Serial.print("start"); 
      while(Firebase.getString("TAG_INFO/MODE") == "")
      {
        Serial.print(".");
        delay(100);
      }
      Serial.println("\r\n");
      
      MODE_GET = Firebase.getString("TAG_INFO/MODE"); 
       
       if(MODE_GET == "REGISTER"){
          while(DB_GET_CNT++ < TOTAL_TAG) 
          {  
               Email_GET = Firebase.getString("Member/User"+String(DB_GET_CNT)+"/email"); 
      
               // Serial.print("Member/User"); Serial.println(String(DB_GET_CNT)+"/email");
                Serial.print("Email_GET:");Serial.println(Email_GET);

              if(Email_GET == "")
              {
                   Serial.print("USER"); Serial.print(DB_GET_CNT);Serial.println(" EMPTY"); 
                   //Serial.print("TAG_INFO/TAG_READ"); Serial.println(comfirmedTag);
      
                   Firebase.setString("TAG_INFO/USER_ORDER",String(DB_GET_CNT));  
                   //delay(1000);
                   Firebase.setString("TAG_INFO/TAG_READ",comfirmedTag);  
                   DB_GET_CNT_STORED = DB_GET_CNT;
                   //MODE_GET ="";
                   break;  
              } 
              else{
                  //Serial.print("USER");Serial.print(DB_GET_CNT); Serial.println(" EXIST");
              }
           }
           DB_GET_CNT = 0;
           MODE_STATE = true;
           Serial.println("end");   
       }
       else if(MODE_GET == "RF_LOG")
       {
           while(DB_GET_CNT++ < TOTAL_TAG) 
            {  
                 TAG_GET = Firebase.getString("Member/User"+String(DB_GET_CNT)+"/tag"); 

                 // Serial.print("Member/User"); Serial.println(String(DB_GET_CNT)+"/email");
                //Serial.print("TAG_GET:");Serial.println(TAG_GET);

                if(TAG_GET != "")
                {
                     if(TAG_GET == comfirmedTag)
                     {
                           String ID_GET = Firebase.getString("Member/User"+String(DB_GET_CNT)+"/email"); 
                           
                           Firebase.setString("LAUNDRY_INFO/ACCOUNT",ID_GET);   
                           Firebase.setString("TAG_INFO/MODE","");  
                           //Firebase.setString("TAG_INFO/RF_STATE","OK");    
                           
                           Serial.print("ID_GET:"); Serial.println(ID_GET); 
                           Serial.print("DB_GET_CNT:"); Serial.println(DB_GET_CNT);   
                           Serial.print("comfirmedTag:"); Serial.println(comfirmedTag);   
                           //MODE_GET = TAG_GET = "";
                           break;
                     }
                }
                else{
                    //Serial.print("USER");Serial.print(DB_GET_CNT); Serial.println(" EXIST");
                }
             }
             DB_GET_CNT = 0; 
             Serial.println("end");   
       }
       else if(MODE_GET == "")
       {
         Serial.println("NOT A NORMAL PROCESS");
       }
    }
    
    if(MODE_STATE == true)
    {    
        Serial.print("waiting");
       // MODE_GET = Firebase.getString("TAG_INFO/MODE"); 
        while(Firebase.getString("TAG_INFO/MODE") != "SET")
        {
          Serial.print(".");
          delay(100);
        }
        Serial.println("\r\n");
        
        Firebase.setString("Member/User"+String(DB_GET_CNT_STORED)+"/tag",comfirmedTag);   
        Serial.print("DB_USER:");Serial.print(DB_GET_CNT_STORED);Serial.print(" DB_TAG:");Serial.println(comfirmedTag);
        Firebase.setString("TAG_INFO/MODE","");   
        DB_GET_CNT_STORED = 0;
        MODE_STATE = false; 
        MODE_GET = ""; 
    }
}

unsigned long TIMER;
String MC_UPDATE;
   
String currentTime;
unsigned long TIMER_CNT;
bool TIMER_STATE;

void loop()
{  
  READ_AND_FIRE(); 
   
}
  
void FIREBASE_SET(){

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);                              // connect to firebase 
   
    for(int j =0; j<TOTAL_TAG; j++)
    {
        tempStr="";
        for(int a = 0; a < 14; a++)
        {
            if(j == 0)
              tempStr += tag1[a];
            else if(j == 1)
              tempStr += tag2[a];  
            else if(j == 2)
              tempStr += tag3[a];  
            else if(j == 3)
              tempStr += tag4[a];
            else
              tempStr += tag5[a];
        }
        //Serial.println(tempStr);
        Firebase.setString("TAG_LIST/tag"+String(j+1),tempStr);             // 등록된 태그 리스트
        Firebase.setString("LAUNDRY_INFO/MC"+String(j+1),"");               // 세탁기 시간값 저장 child값
        delay(100);
    }

    if(Firebase.getString("TAG_INFO/MODE") == "")
    {
        Firebase.setString("LAUNDRY_INFO/ACCOUNT","");               // 세탁기 상태 업데이트 요청 child값
        Firebase.setString("TAG_INFO/USER_ORDER","");            // DB에 빈 유저 넘버자리 찾는 임시 child값
        Firebase.setString("TAG_INFO/MODE","");                   // DB에 현 모드상태(set,log 등) 임시 child값
        Firebase.setString("TAG_INFO/TAG_READ","");               // DB에 현재 읽은 TAG값 넣는 임시 child값
        //Firebase.setString("TAG_INFO/RF_STATE","");               // DB에 현재 태그 리딩 상태값 저장 임시 child값
        Serial.println("TAG INFO INITIALIZED ==============  \r\n");
    }
    else
    { 
        Serial.println("TAG INFO NOT EMPTY ==============  \r\n");
    }
 
}   

void READ_ID()
{
  while(Serial.available())
  { 
      buffer[bufferIndex] = Serial.read();   //시리얼 통신으로 버퍼배열에 데이터 수신
      bufferIndex++;      
      
      int b =atoi(buffer); 
      Serial.println(b);  
      String TAG_GET = Firebase.getString("Member/User"+String(b)+"/ID");
      Serial.println(TAG_GET); 
      bufferIndex =0;
  }
}
