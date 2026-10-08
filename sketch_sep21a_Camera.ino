// ======================================================
// ESP32 + DRV8833
// Suivi horizontal d'une personne
// Communication Bluetooth Classic
// ======================================================

#include "BluetoothSerial.h"

// ======================================================
// OBJET BLUETOOTH CLASSIC
// ======================================================

BluetoothSerial SerialBT;

// Nom Bluetooth de l'ESP32
const char* BT_NAME = "ESP32_TRACKING";

// ======================================================
// BROCHES DRV8833
// ======================================================

const int AIN1 = 25;
const int AIN2 = 26;
const int nSLEEP = 27;

// ======================================================
// PARAMETRES DE COMMANDE
// ======================================================

// Zone morte autour du centre
const float DEAD_ZONE = 0.10;

// PWM minimum pour que le moteur commence à tourner
const int MIN_PWM = 209;

// PWM maximum
const int MAX_PWM = 255;

// ======================================================
// SECURITE COMMUNICATION
// ======================================================

// Si aucune donnée n'est reçue pendant ce délai,
// le moteur s'arrête automatiquement.
const unsigned long COMMUNICATION_TIMEOUT = 500;

// Temps de dernière réception
unsigned long lastReceptionTime = 0;

// ======================================================
// VARIABLES DE RECEPTION
// ======================================================

String receivedData = "";

// ======================================================
// COMMANDE DU MOTEUR
// ======================================================

void moteurStop()
{
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
}


// ======================================================
// MOTEUR GAUCHE
// ======================================================

void moteurGauche(int pwm)
{
  pwm = constrain(pwm, 0, 255);

  digitalWrite(AIN2, LOW);
  analogWrite(AIN1, pwm);
}


// ======================================================
// MOTEUR DROITE
// ======================================================

void moteurDroite(int pwm)
{
  pwm = constrain(pwm, 0, 255);

  digitalWrite(AIN1, LOW);
  analogWrite(AIN2, pwm);
}


// ======================================================
// TRAITEMENT DE LA POSITION X
// ======================================================

void traiterPosition(float x)
{
  // ----------------------------------------------------
  // Sécurité : limitation de x entre -1 et +1
  // ----------------------------------------------------

  x = constrain(x, -1.0, 1.0);

  // Mise à jour du temps de réception
  lastReceptionTime = millis();

  // Affichage
  Serial.print("Position x = ");
  Serial.println(x, 3);

  SerialBT.print("Position x = ");
  SerialBT.println(x, 3);


  // ====================================================
  // PERSONNE AU CENTRE
  // ====================================================

  if (abs(x) < DEAD_ZONE)
  {
    moteurStop();

    Serial.println("CENTRE -> STOP");
  }


  // ====================================================
  // PERSONNE A GAUCHE
  // ====================================================

  else if (x < 0)
  {
    // Transformation :
    // |x| = 0.10 -> PWM minimum
    // |x| = 1.00 -> PWM maximum

    int pwm = map(
      (int)(abs(x) * 1000),
      100,
      1000,
      MIN_PWM,
      MAX_PWM
    );

    pwm = constrain(pwm, MIN_PWM, MAX_PWM);

    moteurGauche(pwm);

    Serial.print("GAUCHE -> PWM = ");
    Serial.println(pwm);
  }


  // ====================================================
  // PERSONNE A DROITE
  // ====================================================

  else
  {
    // Transformation :
    // x = 0.10 -> PWM minimum
    // x = 1.00 -> PWM maximum

    int pwm = map(
      (int)(x * 1000),
      100,
      1000,
      MIN_PWM,
      MAX_PWM
    );

    pwm = constrain(pwm, MIN_PWM, MAX_PWM);

    moteurDroite(pwm);

    Serial.print("DROITE -> PWM = ");
    Serial.println(pwm);
  }
}


// ======================================================
// RECEPTION BLUETOOTH
// ======================================================

void recevoirBluetooth()
{
  while (SerialBT.available())
  {
    char c = SerialBT.read();

    // --------------------------------------------------
    // Fin d'une commande
    // --------------------------------------------------

    if (c == '\n' || c == '\r')
    {
      if (receivedData.length() > 0)
      {
        float x = receivedData.toFloat();

        traiterPosition(x);

        // Effacement du buffer
        receivedData = "";
      }
    }

    // --------------------------------------------------
    // Caractère normal
    // --------------------------------------------------

    else
    {
      receivedData += c;

      // Protection contre une chaîne trop longue
      if (receivedData.length() > 20)
      {
        receivedData = "";
      }
    }
  }
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  // ====================================================
  // PORT SERIE
  // ====================================================

  Serial.begin(115200);

  delay(500);


  // ====================================================
  // CONFIGURATION DES BROCHES
  // ====================================================

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(nSLEEP, OUTPUT);


  // ====================================================
  // ACTIVATION DU DRV8833
  // ====================================================

  digitalWrite(nSLEEP, HIGH);


  // ====================================================
  // MOTEUR ARRETE AU DEMARRAGE
  // ====================================================

  moteurStop();


  // ====================================================
  // INITIALISATION BLUETOOTH CLASSIC
  // ====================================================

  if (!SerialBT.begin(BT_NAME))
  {
    Serial.println("ERREUR : impossible de demarrer Bluetooth");
    return;
  }


  // ====================================================
  // INFORMATIONS
  // ====================================================

  Serial.println();
  Serial.println("====================================");
  Serial.println("       ESP32 TRACKING");
  Serial.println("       BLUETOOTH CLASSIC");
  Serial.println("====================================");

  Serial.print("Nom Bluetooth : ");
  Serial.println(BT_NAME);

  Serial.println("Bluetooth Classic disponible");
  Serial.println("En attente d'une connexion...");
  Serial.println();
  Serial.println("Format attendu :");
  Serial.println("x = -1.0 a +1.0");
  Serial.println("Exemple : -0.50");
  Serial.println();
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
  // ====================================================
  // RECEPTION DES DONNEES BLUETOOTH
  // ====================================================

  recevoirBluetooth();


  // ====================================================
  // SECURITE : TIMEOUT COMMUNICATION
  // ====================================================

  if (millis() - lastReceptionTime > COMMUNICATION_TIMEOUT)
  {
    moteurStop();
  }


  delay(5);
}