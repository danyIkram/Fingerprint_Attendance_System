#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Création de l'objet pour le capteur d'empreintes
SoftwareSerial mySerial(2, 3); // RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
//modif sans fil : 
SoftwareSerial btSerial(7, 8); // RX, TX du module Bluetooth
//

// Création de l'objet LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adresse I2C 0x27, 16 colonnes et 2 lignes

// Définition des broches pour les boutons
#define ADD_BUTTON 4
#define SEARCH_BUTTON 5
#define EMPTY_BUTTON 6

// Variables pour l'état des boutons
int lastAddState = HIGH;
int lastSearchState = HIGH;
int lastEmptyState = HIGH;
int currentAddState;
int currentSearchState;
int currentEmptyState;

void setup() {
  Serial.begin(9600);
  mySerial.listen();      // capteur d'empreintes
  finger.begin(57600);
  /*modif sans fil : 
  btSerial.begin(9600); // Vitesse standard du HC-05
  */

  if (finger.verifyPassword()) {
    Serial.println("Capteur d'empreintes trouvé!");
  } else {
    Serial.println("Capteur d'empreintes non trouvé :(");
    while (1) { delay(1); }
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Systeme Presence");
  lcd.setCursor(0, 1);
  lcd.print("Pret a l'emploi");

  pinMode(ADD_BUTTON, INPUT_PULLUP);
  pinMode(SEARCH_BUTTON, INPUT_PULLUP);
  pinMode(EMPTY_BUTTON, INPUT_PULLUP);
  
  delay(2000);
}

void loop() {
  currentAddState = digitalRead(ADD_BUTTON);
  currentSearchState = digitalRead(SEARCH_BUTTON);
  currentEmptyState = digitalRead(EMPTY_BUTTON);

  if (lastAddState == HIGH && currentAddState == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode Ajout");
    lcd.setCursor(0, 1);
    lcd.print("Nouvel etudiant");
    delay(1000);
    enrollFingerprint();
  }

  if (lastSearchState == HIGH && currentSearchState == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode Recherche");
    lcd.setCursor(0, 1);
    lcd.print("Placez le doigt");
    delay(1000);
    searchFingerprint();
  }

  if (lastEmptyState == HIGH && currentEmptyState == LOW) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Mode Suppression");
    lcd.setCursor(0, 1);
    lcd.print("Tout effacer?");
    delay(2000);
    
    if (digitalRead(EMPTY_BUTTON) == LOW) {
      emptyDatabase();
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Annule");
      delay(1000);
    }
  }

  lastAddState = currentAddState;
  lastSearchState = currentSearchState;
  lastEmptyState = currentEmptyState;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Systeme Presence");
  lcd.setCursor(0, 1);
  lcd.print("Pret a l'emploi");

  delay(100);
}

uint8_t enrollFingerprint() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ajout empreinte");

  uint8_t id = findFreeID();
  if (id == 0xFF) {
    lcd.setCursor(0, 1);
    lcd.print("Memoire pleine!");
    delay(2000);
    return 0xFF;
  }

  lcd.setCursor(0, 1);
  lcd.print("ID #");
  lcd.print(id);
  delay(2000);

  int p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Placez doigt");
  lcd.setCursor(0, 1);
  lcd.print("ID #");
  lcd.print(id);

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) continue;
    if (p == FINGERPRINT_PACKETRECIEVEERR || p == FINGERPRINT_IMAGEFAIL) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erreur capture");
      delay(1000);
      return 0xFF;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur conv.");
    delay(1000);
    return 0xFF;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Retirez doigt");
  delay(2000);

  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Replacez doigt");

  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_PACKETRECIEVEERR || p == FINGERPRINT_IMAGEFAIL) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Erreur capture");
      delay(1000);
      return 0xFF;
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur conv.2");
    delay(1000);
    return 0xFF;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Non identique");
    delay(2000);
    return 0xFF;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Succes! ID #");
    lcd.print(id);
    /*modif sans fil : 
    btSerial.listen();
    btSerial.print("ENROLL:ID=");
    btSerial.println(id); //envoi vers bluetooth
    */
    delay(2000);
    return id;
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Echec stockage");
    delay(1000);
    return 0xFF;
  }
}

uint8_t searchFingerprint() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Recherche...");

  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      if (digitalRead(ADD_BUTTON) == LOW || digitalRead(SEARCH_BUTTON) == LOW || digitalRead(EMPTY_BUTTON) == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Annule");
        delay(1000);
        return 0xFF;
      }
    }
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur conv.");
    delay(1000);
    return 0xFF;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Trouve! ID #");
    lcd.print(finger.fingerID);
    lcd.setCursor(0, 1);
    lcd.print("Confiance: ");
    lcd.print(finger.confidence);
    /*modif sans fil 
    btSerial.print("FOUND:ID=");
    btSerial.println(finger.fingerID); // Envoi vers Bluetooth
    */
    delay(3000);
    return finger.fingerID;
  } else if (p == FINGERPRINT_NOTFOUND) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Non trouve");
    delay(2000);
    return 0xFF;
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erreur recherche");
    delay(1000);
    return 0xFF;
  }
}

void emptyDatabase() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suppression...");

  for (int i = 1; i < 127; i++) {
    int p = finger.deleteModel(i);
    delay(50); // Petite pause pour stabilité
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BD vide!");
  delay(2000);
}

// Fonction utilitaire pour trouver un ID libre
uint8_t findFreeID() {
  for (uint8_t id = 1; id < 127; id++) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      return id;
    }
  }
  return 0xFF; // Aucun ID libre
}


