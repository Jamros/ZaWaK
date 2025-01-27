//---------------------------------------------------------------------------------
//-------------------------INCLUDE-------------------------------------------------
//---------------------------------------------------------------------------------

#include "HX711.h"
#include <U8g2lib.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <HTTPClient.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
#endif

#include <ArduinoJson.h>

//---------------------------------------------------------------------------------
//----------------------JSON Parametrs--------------------------------------------
//---------------------------------------------------------------------------------

const size_t capacity = JSON_ARRAY_SIZE(10) + 10*JSON_OBJECT_SIZE(1) + 190;
//---------------------------------------------------------------------------------
//----------------------Wi-Fi Parametrs--------------------------------------------
//---------------------------------------------------------------------------------

const char* ssid = "AndroidAP3348";
const char* password = "gase4288";
//---------------------------------------------------------------------------------
//----------------------HX711 circuit wiring---------------------------------------
//---------------------------------------------------------------------------------

const int LOADCELL_DOUT_PIN = 4;
const int LOADCELL_SCK_PIN = 15;
HX711 scale;

//---------------------------------------------------------------------------------
//----------------------CONST DLA KALIBRACJI---------------------------------------
//---------------------------------------------------------------------------------

const float calibrationValue = -126.06;
//const float calibrationValue = -1.0;

//---------------------------------------------------------------------------------
//---------------------INICJALIZACJA OLEDA-----------------------------------------
//---------------------------------------------------------------------------------

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, 
                                         /* reset=*/ U8X8_PIN_NONE);

const char *string_list[] = 
  {"Piersc z kurczaka\n",
  "Ryz\n",
  "Makaron\n",
  "Pomidor\n",
  "Ogorek\n",
  "Szynka\n",
  "Kapusta\n",
  "Bulka\n",
  "Chleb\n",
  "Malina"};

uint8_t current_selection = 0;

//---------------------------------------------------------------------------------
//--------------------INTERRUPTPIN FOR BUTTON--------------------------------------
//---------------------------------------------------------------------------------

const int ButtonSelect = 5;
const int ButtonRight = 18;
const int ButtonLeft = 19;
const int ButtonOnOff = 23;

const int ButtonS = 1;
const int ButtonR = 2;
const int ButtonL = 3;
const int ButtonO = 4;

//----------------------------------------------------------------------------------
//-------------------INICJALIZACJA KOLEJKI------------------------------------------
//----------------------------------------------------------------------------------

QueueHandle_t serialQueue1;
QueueHandle_t serialQueue2;
QueueHandle_t WiFiQueue;

//----------------------------------------------------------------------------------
//-------------------INICJALIZACJA SEMAFORU-----------------------------------------
//----------------------------------------------------------------------------------

//SemaphoreHandle_t sem_1;

//----------------------------------------------------------------------------------
//---------------------FUNKCJA PRZERWANIA-------------------------------------------
//----------------------------------------------------------------------------------

void interruptHandler() { 
  BaseType_t sHigherPriorityTaskWoken = pdFALSE;

    if (digitalRead(ButtonSelect) == LOW) {
      xQueueSend(serialQueue2,&ButtonSelect,portMAX_DELAY);
    }
    else if (digitalRead(ButtonRight) ==LOW){
      Serial.println("Right");
      xQueueSend(serialQueue2,&ButtonRight,portMAX_DELAY);
    }
    else if (digitalRead(ButtonLeft) ==LOW){
      xQueueSend(serialQueue2,&ButtonLeft,portMAX_DELAY);
    }
    else if (digitalRead(ButtonOnOff) ==LOW){
      xQueueSend(serialQueue2,&ButtonOnOff,portMAX_DELAY);
    }
 
    portYIELD_FROM_ISR(  );
}

//----------------------------------------------------------------------------------
//---------------------FUNKCJA PRZYCISKU--------------------------------------------
//----------------------------------------------------------------------------------



//----------------------------------------------------------------------------------
//------------------------FUNKCJA MENU----------------------------------------------
//----------------------------------------------------------------------------------


//----------------------------------------------------------------------------------
//------------------------FUNKCJA EKRAN STARTOWY------------------------------------
//----------------------------------------------------------------------------------
void Start(float fCurrentWeight1) {
      char charVal[10]; 
      dtostrf(round(abs(fCurrentWeight1)), 4, 0, charVal);
      u8g2.clearBuffer();                      
      u8g2.setFont(u8g2_font_ncenB12_tr);     
      u8g2.drawStr(60,41,charVal);
      u8g2.setFont(u8g2_font_ncenB14_tr);             
      u8g2.drawStr(30,20,"ZaWaK"); 
      u8g2.setFont(u8g2_font_ncenB12_tr);              
      u8g2.drawStr(0,41,"Waga: ");
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0,62,"Menu");
      u8g2.drawCircle(45,58,5, U8G2_DRAW_ALL);
      u8g2.drawCircle(60,58,5, U8G2_DRAW_ALL);
      u8g2.drawCircle(75,58,5, U8G2_DRAW_ALL);
      u8g2.drawCircle(90,58,5, U8G2_DRAW_ALL);
      u8g2.sendBuffer();
  }


//----------------------------------------------------------------------------------
//------------------------FUNKCJA KLIK MENU-----------------------------------------
//----------------------------------------------------------------------------------

int KlikPrzycisk (float fCurrentWeight1,boolean &StartMenu,boolean &MenuWybranyProdukt){
  
static int i=0;
int queueValue;

u8g2.clearBuffer();
u8g2.setFont(u8g2_font_ncenB10_tr);
u8g2.drawStr(10,20,string_list[i]);
u8g2.sendBuffer();
if (xQueueReceive(serialQueue2,&queueValue,(TickType_t)10) == pdTRUE){

  if (queueValue == ButtonRight){
              i++; 
            }
  else if (queueValue == ButtonLeft){
              i--;
            }
  else if (queueValue == ButtonOnOff){
          StartMenu = true;
          MenuWybranyProdukt = false;  
  }
  else if (queueValue == ButtonSelect){
          MenuWybranyProdukt = true;
           
  }
}
return i; 
}

//----------------------------------------------------------------------------------
//-------------------------FUNKCJA WYBRANEGO PRODUKTU-------------------------------
//----------------------------------------------------------------------------------

void WybranyProdukt (float fCurrentWeight1, boolean &StartMenu, int wybor,boolean &MenuWybranyProdukt){

  char charVal[10]; 
  dtostrf(round(abs(fCurrentWeight1)), 4, 0, charVal);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);         
  u8g2.drawStr(44,18,charVal);               
  u8g2.drawStr(44,8,"ZaWaK");               
  u8g2.drawStr(0,17,"Waga: ");  
  u8g2.drawStr(0,26,"Produkt: ");
  u8g2.drawStr(50,26, string_list[wybor]);                      
  u8g2.drawStr(0,35,"Kalorie: "); 
  u8g2.drawStr(0,44,"Weglowodany: ");
  u8g2.drawStr(0,53,"Tluszcze: ");
  u8g2.drawStr(0,62,"Bialko: ");
  u8g2.sendBuffer();

int queueValue;
if (xQueueReceive(serialQueue2,&queueValue,(TickType_t)10) == pdTRUE){
  if (queueValue == ButtonOnOff){
         MenuWybranyProdukt = false; 
         StartMenu = false;
         

  }
}
} 

  


//----------------------------------------------------------------------------------
//-------------------------FUNKCJA INICJILIZACIJNA----------------------------------
//----------------------------------------------------------------------------------

void setup() {
  
  pinMode(ButtonSelect, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ButtonSelect), interruptHandler, FALLING);
  
  pinMode(ButtonRight, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ButtonRight), interruptHandler, FALLING);

  pinMode(ButtonLeft, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ButtonLeft), interruptHandler, FALLING);

  pinMode(ButtonOnOff, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ButtonOnOff), interruptHandler, FALLING);
  
  Serial.begin(115200);
  
  while (!Serial) {
      ;  
    }
    u8g2.begin();
     

//---------------------------KOLEJEKI-----------------------------------------------

  serialQueue1 = xQueueCreate(16, sizeof(float));
  if(serialQueue1 == NULL)
  {
    Serial.println("Failed to createQueeue1");
    while(true);
  }

  serialQueue2 = xQueueCreate(16, sizeof(int));
  if(serialQueue2 == NULL)
  {
    Serial.println("Failed to createQueeue2");
    while(true);
  }

  WiFiQueue = xQueueCreate(30, sizeof(char));
  if(WiFiQueue == NULL)
  {
    Serial.println("Failed to WiFiQueue");
    while(true);
  }
  
//----------------------------------------------------------------------------------

//---------------------------SEMAFOR------------------------------------------------

/*
if ( sem_1 == NULL )                                      // Check to confirm that the Serial Semaphore has not already been created.
  {
    sem_1 = xSemaphoreCreateBinary();                     // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( sem_1 ) != NULL )
      xSemaphoreGive( ( sem_1 ) );                        // Make the Serial Port available for use, by "Giving" the Semaphore.
  }

*/
//---------------------------------------------------------------------------------

//-------------------------HX711---------------------------------------------------
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibrationValue);                  
  scale.tare();                                       
//---------------------------------------------------------------------------------


//---------------------------TASKI-------------------------------------------------

 xTaskCreatePinnedToCore(
      HX711_code, /* Функция, содержащая код задачи */
      "HX711_Task", /* Название задачи */
      8192, /* Размер стека в словах */
      NULL, /* Параметр создаваемой задачи */
      2, /* Приоритет задачи */
      NULL, /* Идентификатор задачи */
      1); /* Ядро, на котором будет выполняться задача */

 xTaskCreatePinnedToCore(
      OLED_code, 
      "OLED_Task", 
      8192, 
      NULL, 
      1, 
      NULL, 
      0); 
  
 
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
//-------------------------TASK HX711---------------------------------------------
//--------------------------------------------------------------------------------

static void HX711_code(void* parameter) {
  for(;;)
  {
  float weight;
  //Serial.print("one reading:\t");
  weight = scale.get_units();
  //Serial.println(weight, 1);
  xQueueSend(serialQueue1,&weight,portMAX_DELAY);
  scale.power_down();              // put the ADC in sleep mode
  vTaskDelay((200L * configTICK_RATE_HZ) / 1000L); // wait for 200ms
  scale.power_up();
}
}
//--------------------------------------------------------------------------------
//-------------------------TASK OLED----------------------------------------------
//--------------------------------------------------------------------------------

void OLED_code(void* parameter) {
 static float fCurrentWeight1 = 0.0;
 static boolean StartMenu = true;
 static boolean MenuWybranyProdukt = false;
 static int wybor;
 while (1){
  
if (StartMenu && !MenuWybranyProdukt) 
{
 Serial.println("1");
  Start(fCurrentWeight1);
    int queueValue;
    if (xQueueReceive(serialQueue2,&queueValue,(TickType_t)10) == pdTRUE){
        StartMenu = false;
      
     }
}
else if(!StartMenu && !MenuWybranyProdukt)
{
   Serial.println("2");
  wybor = KlikPrzycisk(fCurrentWeight1,StartMenu, MenuWybranyProdukt);
}
else if (MenuWybranyProdukt) {
   Serial.println("3");
  WybranyProdukt(fCurrentWeight1, StartMenu, wybor, MenuWybranyProdukt);
}
xQueueReceive(serialQueue1,&fCurrentWeight1,(TickType_t)10);

}

}

//--------------------------------------------------------------------------------
//-------------------------TASK WI-FI----------------------------------------------
//--------------------------------------------------------------------------------
void WiFi_Code(void* pvParametrs)
{
    char* product_name;
    if (xQueueReceive(WiFiQueue,&product_name,(TickType_t)10) == pdTRUE){
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) 
      {
        vTaskDelay((1000L * configTICK_RATE_HZ) / 1000L); // wait for 200ms
        Serial.print("Connecting..");
      }
      HTTPClient http;  //Declare an object of class HTTPClient
      char* ServerName = "http://server194717.nazwa.pl/ZaWaK/get-json.php?api_key=tPmAT5Ab3j7F9&product_name=";
      strncat(ServerName,product_name,114);
      http.begin(ServerName);  //Specify request destination
      int httpCode = http.GET();     //Send the request                                                           
  
      if (httpCode > 0) { //Check the returning code
        String payload = http.getString();   //Get the request response payload
        Serial.println(payload);                     //Print the response payload
  
        DynamicJsonDocument doc(capacity);
     
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, payload);
      
        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }
      }
  
      http.end();   //Close connection

    } 
    
  }
void loop()
{
  
}
