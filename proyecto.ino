#include <RTClib.h>

#include <Servo.h>

#include <EEPROM.h>

#include <Wire.h>

#include <AT24CX.h>

#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <deprecated.h>
#include <require_cpp11.h>

#include <Keypad.h>
#include <Key.h>



#include <LiquidCrystal.h>

#define RST_PIN   5
#define SS_PIN    53

RTC_DS1307 rtc;
AT24C32 EepromRTC;

Servo servo;

MFRC522 mfrc522(SS_PIN, RST_PIN); 

/*===========================
Conection LCD to Arduino
=============================*/

/*
  LCD RS to PIN 14
  LCD E to PIN 15
  LCD D4 to PIN 16
  LCD D5 to PIN 17
  LCD D6 to PIN 18
  LCD D7 to PIN 19
*/
LiquidCrystal lcd(14, 15, 16, 17, 18, 19);

/*==========================
 * Configuration keyboard
============================*/

/*
*/
const byte rows = 4;
const byte cols = 4;
char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},  
};
/*
  Filas y columnas a las que van conectado el teclado
*/
byte rowPins[rows] = {10,9,8,7};
byte colPins[cols] = {6,4,3,2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

/*==================================
 * Global Variables
====================================*/
bool pass = false, menu = false;
byte tarjeta[4] = {0x87, 0x49, 0xC1, 0x4D};
byte lecturaUID[4];
int digito = 0, reporte[7], contador, opc, posicion, dato;
short pos_libre;
int pinServo = 11;
int PulsoMin = 1000;
int PulsoMax = 2000;
int rojo = 12;
int verde = 13;
char codigo[4], password[] = "1234", admin[] = "4321", opcMenu[] = "12";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Lcd
  lcd.begin(16, 2);
  // rtc
  rtc.begin();
  //rtc.adjust(DateTime(__DATE__,__TIME__));
  DateTime now = rtc.now();
  Serial.println("Date: ");
  Serial.println(now.day());
  Serial.println(now.month());
  Serial.println(atoi(now.toString("YY")));
  Serial.print("Hour: ");
  Serial.println(now.hour());
  Serial.println(now.minute());
  Serial.println(now.second());
  // Servo
  servo.attach(pinServo, PulsoMin, PulsoMax);
  delay(1000);
  servo.write(200);

  // Limpiar eeprom externa
  /*
    for (int i = 0; i<280; i++) {
    EepromRTC.write(i,0);
  }
  byte a = EepromRTC.read(1);
  Serial.print("Dato byte: ");Serial.println(a);
  */
  
  // lector tarjeta
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  pinMode(rojo, OUTPUT);
  pinMode(verde, OUTPUT);
  bienvenida();
}

void loop() {
  // put your main code here, to run repeatedly:
}

/*==============================================
FUNCIONES DEL TECLADO
================================================*/
void pedirPass() {
  while(pass) {
    if ( mfrc522.PICC_IsNewCardPresent()) {
    
     if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }
  
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        lecturaUID[i] = mfrc522.uid.uidByte[i];
      }
      if (comparaUID(lecturaUID, tarjeta)) {
        Serial.println("Correcto");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Siga");
        buscar_pos_libre();
        if (pos_libre == true) {
          DateTime now = rtc.now();
          reporte[0] = contador;
          reporte[1] = now.day();
          reporte[2] = now.month();
          reporte[3] = atoi(now.toString("YY"));
          reporte[4] = now.hour();
          reporte[5] = now.minute();
          reporte[6] = now.second();

          for (int i = 0; i < 7; i++) {   
            EepromRTC.write(posicion,(byte *)reporte[i]);               
            delay(5);
            posicion = posicion + 1;
          }
        }
        digitalWrite(verde, HIGH);
        digitalWrite(rojo, LOW);
        abrir();
        pass = false;      
        cerrar();
        delay(1000);
        digitalWrite(verde, LOW);
        digitalWrite(rojo, HIGH);
        delay(500);
        bienvenida();
      } else {    
        Serial.println("Incorrecto");
        pass = false;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Incorrecto");
        delay(1500);
        pedirPinOTarjeta();
      }
      mfrc522.PICC_HaltA(); 
  } else {
      char tecla = keypad.getKey();
      if (tecla != NO_KEY) {
        codigo[digito] = tecla;
        digito++;
        if (digito == 4) {
          if (comparaPassword()) {
            Serial.println("Correcto");
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Siga");
            digitalWrite(verde, HIGH);
            digitalWrite(rojo, LOW);
            buscar_pos_libre();
            if (pos_libre == true) {
              DateTime now = rtc.now();
              reporte[0] = contador;
              reporte[1] = now.day();
              reporte[2] = now.month();
              reporte[3] = atoi(now.toString("YY"));
              reporte[4] = now.hour();
              reporte[5] = now.minute();
              reporte[6] = now.second();

              for (int i = 0; i < 7; i++) {   
                EepromRTC.write(posicion,(byte *)reporte[i]);               
                delay(5);
                posicion = posicion + 1;
              }
            }
            abrir();
            pass = false;
            cerrar();            
            delay(1000);
            digitalWrite(verde, LOW);
            digitalWrite(rojo, HIGH);
            delay(500);
            bienvenida();
          } else {
            if (compareAdmin()) {
              pass = false;
              digitalWrite(verde, HIGH);
              digitalWrite(rojo, LOW);
              delay(1000);
              digitalWrite(verde, LOW);
              digitalWrite(rojo, HIGH);
              menuAdmin();
              Serial.println("Correcto");
            } else {
              pass = false;
              
              Serial.println("Incorrecto");
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print("Incorrecto");
              delay(1500);
              pedirPinOTarjeta();
            }
          }
        }
      }
    }
  }
  
}

void getOptMenu() {
  char tecla = keypad.getKey();
  if (tecla != NO_KEY) {
    opc = tecla;
    digito++;
    delay(50);
    if (digito == 1) {
      menu = false;
      if (opc == opcMenu[0]) {
        getReports();
      } 

      if (opc == opcMenu[1]) {
        bienvenida();
      }
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Opcion no valida");
      delay(1500);
      menuAdmin();
    }
  }
}

/*===============================================
TERMINAN FUNCIONES DEL TECLADO
=================================================*/

/*===============================================
 * COMPARACIONES  
=================================================*/
boolean comparaUID(byte lectura[], byte usuario[]) {
  for (byte i=0; i<mfrc522.uid.size; i++) {
    if (lectura[i] != usuario[i]) {
      return (false);
    } 
  }
  return (true);
}

boolean comparaPassword() {
  if (codigo[0] != password[0] && codigo[1] != password[1] && codigo[2] != password[2] && codigo[3] != password[3]) {
    return (false);
  }

  return (true);
}

boolean compareAdmin() {
  if (codigo[0] != admin[0] && codigo[1] != admin[1] && codigo[2] != admin[2] && codigo[3] != admin[3]) {
    return (false);
  }

  return (true);
}

/*===============================================
BIENVENIDA
=================================================*/
void bienvenida() {
  digitalWrite(rojo, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Bienvenido al");
  lcd.setCursor(0,1);
  lcd.print("parqueadero SAM");
  delay(3000);
  pedirPinOTarjeta();
}

/*===============================================
PEDIR PIN O TARJETA
=================================================*/
void pedirPinOTarjeta() {
  digito = 0;
  codigo[0] = "";
  codigo[1] = "";
  codigo[2] = "";
  codigo[3] = "";
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ingrese pin o");
  lcd.setCursor(0,1);
  lcd.print("pase la tarjeta");
  pass = true;
  delay(4000);
  pedirPass();
}

/*===============================================
ABRIR SERVO
=================================================*/
void abrir() {
  servo.write(200);
  delay(1000);
  servo.write(-180);
}
/*===============================================
CERRAR EL SERVO
=================================================*/
void cerrar() {
  delay(5000);
  servo.write(200);
}

void menuAdmin() {
  digito = 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("1).Ver reportes");
  lcd.setCursor(0,1);
  lcd.print("2).Salir");
  menu = true;
  while (menu) {
    getOptMenu();
  }
}
/*===============================================
OBTENER REPORTES
=================================================*/
void getReports() {
  const char* opts[] = {"REPORTE", "Day", "Month", "Year", "Hour", "Minute", "Second"};
  int add = 0;
  int opt = 0;
  byte value = EepromRTC.read(add);
  while(value != 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(opt[opts]);
    lcd.print(" ");
    lcd.print(value);
    delay(1000);

    lcd.clear();

    add++;
    opt++;
    value = EepromRTC.read(add);     
    if (add == 7 || add == 14 || add == 21 || add == 28 || add == 35 || add == 42 || add == 49 || add == 56 || add == 63 || add == 70 || add == 77 || add == 84 || add == 91 || add == 98 || add == 105) {
      opt = 0;
    }
  }
  delay(100);
  lcd.print("Fin...");
  delay(1000);
  menuAdmin();
}
/*===============================================
BUSCAR POS LIBRE
=================================================*/
void buscar_pos_libre() {
  posicion = 0;
  pos_libre = false;
  contador = 0;
  do {
      
    dato = EepromRTC.read(posicion);
      
    if (dato == 0) {
      pos_libre = true;
    } else {
      posicion += 7;      
    }
    contador++;
  } while (pos_libre == false);
}
