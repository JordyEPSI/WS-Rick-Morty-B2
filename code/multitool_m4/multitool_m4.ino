#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15
#define D0 16

#define pinBTN D6
#define pinLED D4
#define pinLASER D0
#define pinBUZZER D5
#define pinLED_R 1  // TX / GPIO1 : ne pas utiliser Serial.
#define pinLED_V D8

#define TRIG D3
#define ECHO D7     // ECHO via un diviseur de tension 5 V vers 3,3 V.

#define ALLUME_LED LOW
#define ETEIND_LED HIGH

#define ALLUME_LASER HIGH
#define ETEIND_LASER LOW

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool systemeActive = false;
int derniereLecture = HIGH;
int etatBouton = HIGH;
unsigned long dernierChangement = 0;
unsigned long derniereMesure = 0;
const unsigned long DUREE_CHARGE = 3000;
unsigned long debutCharge = 0;
unsigned long dernierEffet = 0;
const unsigned long DUREE_PAUSE = 80;
const unsigned long DUREE_SON_TIR = 180;
const unsigned long DELAI_VEILLE_AUTO = 120000;
unsigned long derniereActivite = 0;
const unsigned int FREQUENCE_CHARGE_DEBUT = 350;
const unsigned int FREQUENCE_CHARGE_FIN = 1800;
const unsigned int FREQUENCE_TIR_DEBUT = 3000;
const unsigned int FREQUENCE_TIR_FIN = 180;
unsigned long debutPhase = 0;
unsigned long debutPulsation = 0;
bool feuAffiche = false;
const unsigned long DELAI_DOUBLE_CLIC = 650;
const unsigned long DUREE_CLIC_COURT = 500;
const unsigned long DUREE_TIR_AUTO = 90;
const unsigned long PERIODE_TIR_AUTO = 240;
const int CAPACITE = 20;
const int VERT_ORANGE = 90;
const unsigned long DUREE_RECHARGE = 2000;
const unsigned long DUREE_CONFIRMATION = 800;
const unsigned long DUREE_BIP_PRET = 80;
const unsigned long DUREE_VERDICT = 3000;
// Petit carillon de validation original pour le verdict DRIP.
const unsigned int NOTES_VALIDATION[] = {784, 988, 1319, 1568};
const unsigned long DUREES_VALIDATION[] = {80, 90, 120, 220};
const int NOMBRE_NOTES_VALIDATION = sizeof(NOTES_VALIDATION) / sizeof(NOTES_VALIDATION[0]);
const unsigned int NOTES_REFUS[] = {1568, 1047, 698, 392};
const unsigned long DUREES_REFUS[] = {80, 100, 140, 260};
const int NOMBRE_NOTES_REFUS = sizeof(NOTES_REFUS) / sizeof(NOTES_REFUS[0]);
const char* const MESSAGES_DRIP[] = {
  "Sexyyy nana!", "Sapeee!", "Hiiii trop frais", "Popopo ca claque"
};
const char* const MESSAGES_PAS_DRIP[] = {
  "Peut mieux faire", "Change la paire", "Non."
};
int munitions = CAPACITE;
int clicsCourts = 0;
bool verdictAffiche = false;
bool verdictDrip = false;
unsigned long debutVerdict = 0;
int indexVerdict = 0;
bool sonVerdictActif = false;
int noteVerdict = 0;
unsigned long debutNoteVerdict = 0;
unsigned long dernierCycleAuto = 0;
bool tirCycleActif = false;
bool attendreRelachement = false;
unsigned long dernierRelachement = 0;
enum EtatTir { REPOS, CHARGEMENT, PAUSE_TIR, TIR, TIR_AUTO, ATTENTE_CLIC, RECHARGE, CONFIRMATION, VIDE, VEILLE };
EtatTir etatTir = REPOS;
int dernierPourcentage = -1;
unsigned long dernierAffichageCharge = 0;
const unsigned long DUREE_DEMARRAGE = 2000;
const unsigned long DUREE_REVEIL = 2000;
const unsigned long ANTI_REBOND = 30;
unsigned long debutAppui = 0;

void animerDemarrage() {
  digitalWrite(pinLASER, ETEIND_LASER);
  analogWrite(pinLED_R, 0);
  analogWrite(pinLED_V, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INITIALISATION  ");

  unsigned long debut = millis();
  int dernierProgres = -1;
  int dernierSon = -1;
  while (millis() - debut < DUREE_DEMARRAGE) {
    unsigned long ecoule = millis() - debut;
    int progres = ecoule * 100UL / DUREE_DEMARRAGE;
    analogWrite(pinLED_V, ecoule * 255UL / DUREE_DEMARRAGE);

    int son = ecoule < 60 ? 900 :
              (ecoule >= 700 && ecoule < 760 ? 1200 :
              (ecoule >= 1400 && ecoule < 1460 ? 1600 : 0));
    if (son != dernierSon) {
      if (son > 0) tone(pinBUZZER, son);
      else noTone(pinBUZZER);
      dernierSon = son;
    }

    if (progres / 5 != dernierProgres / 5 || dernierProgres < 0) {
      dernierProgres = progres;
      lcd.setCursor(0, 1);
      lcd.print("[");
      for (int i = 0; i < 10; i++) {
        lcd.print(i < progres / 10 ? "#" : " ");
      }
      lcd.print("]");
      if (progres < 100) lcd.print(" ");
      if (progres < 10) lcd.print(" ");
      lcd.print(progres);
      lcd.print("%");
    }
    delay(5);
  }
  noTone(pinBUZZER);
  analogWrite(pinLED_V, 255);
  attendreRelachement = true;
}

void setup() {

  pinMode(pinBTN, INPUT_PULLUP);

  pinMode(pinLED, OUTPUT);
  pinMode(pinLASER, OUTPUT);
  pinMode(pinBUZZER, OUTPUT);

  pinMode(pinLED_R, OUTPUT);
  pinMode(pinLED_V, OUTPUT);
  analogWriteRange(255);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);
  digitalWrite(pinBUZZER, LOW);

  digitalWrite(pinLED, ETEIND_LED);
  digitalWrite(pinLASER, ETEIND_LASER);
  digitalWrite(pinLED_R, LOW);
  digitalWrite(pinLED_V, LOW);

  Wire.begin(D2, D1);

  lcd.init();
  lcd.backlight();

  animerDemarrage();
  derniereActivite = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEME PRET");
  lcd.setCursor(0, 1);
  lcd.print("MUNITIONS 20/20");
}

void couperTir() {
  digitalWrite(pinLASER, ETEIND_LASER);
  digitalWrite(pinLED, ETEIND_LED);
  noTone(pinBUZZER);
  sonVerdictActif = false;
}

void lancerSonVerdict(unsigned long maintenant) {
  noteVerdict = 0;
  debutNoteVerdict = maintenant;
  sonVerdictActif = true;
  tone(pinBUZZER, verdictDrip ? NOTES_VALIDATION[0] : NOTES_REFUS[0]);
}

void actualiserSonVerdict(unsigned long maintenant) {
  if (!sonVerdictActif) return;
  unsigned long duree = verdictDrip ? DUREES_VALIDATION[noteVerdict]
                                    : DUREES_REFUS[noteVerdict];
  if (maintenant - debutNoteVerdict < duree) return;

  debutNoteVerdict = maintenant;
  noteVerdict++;
  int nombreNotes = verdictDrip ? NOMBRE_NOTES_VALIDATION : NOMBRE_NOTES_REFUS;
  if (noteVerdict >= nombreNotes) {
    noTone(pinBUZZER);
    sonVerdictActif = false;
  } else {
    tone(pinBUZZER, verdictDrip ? NOTES_VALIDATION[noteVerdict]
                               : NOTES_REFUS[noteVerdict]);
  }
}

void changerEtat(EtatTir nouvelEtat) {
  etatTir = nouvelEtat;
  feuAffiche = false;
  dernierPourcentage = -1;
}

void afficherChargeurVide() {
  analogWrite(pinLED_R, 255);
  analogWrite(pinLED_V, VERT_ORANGE);  // Rouge + un peu de vert = orange.
}

void signalerVide() {
  couperTir();
  afficherChargeurVide();
  changerEtat(VIDE);
}

void entrerEnVeille() {
  couperTir();
  analogWrite(pinLED_R, 0);
  analogWrite(pinLED_V, 0);
  digitalWrite(TRIG, LOW);
  clicsCourts = 0;
  attendreRelachement = false;
  changerEtat(VEILLE);
  lcd.clear();
  lcd.noDisplay();
  lcd.noBacklight();
}

void reveiller() {
  derniereActivite = millis();
  changerEtat(REPOS);
  clicsCourts = 0;
  attendreRelachement = true;
  systemeActive = false;
  if (munitions == 0) afficherChargeurVide();
  else analogWrite(pinLED_V, 255);
  lcd.display();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEME PRET");
  lcd.setCursor(0, 1);
  lcd.print("MUNITIONS ");
  lcd.print(munitions);
  lcd.print("/");
  lcd.print(CAPACITE);
}

void commencerRafale(unsigned long maintenant) {
  if (munitions == 0) {
    signalerVide();
    return;
  }
  changerEtat(TIR_AUTO);
  munitions--;
  tirCycleActif = true;
  debutPhase = maintenant;
  dernierCycleAuto = 0;
  dernierEffet = maintenant;
  digitalWrite(pinLASER, ALLUME_LASER);
  digitalWrite(pinLED, ALLUME_LED);
  analogWrite(pinLED_R, 255);
  analogWrite(pinLED_V, 0);
  tone(pinBUZZER, FREQUENCE_TIR_DEBUT);
}

void actualiserCharge() {
  int lecture = digitalRead(pinBTN);
  unsigned long maintenant = millis();

  actualiserSonVerdict(maintenant);

  if (verdictAffiche && maintenant - debutVerdict >= DUREE_VERDICT) {
    verdictAffiche = false;
    feuAffiche = false;
    dernierPourcentage = 101;
  }

  if (lecture != derniereLecture) {
    dernierChangement = maintenant;
    derniereLecture = lecture;
  }

  if (etatTir == VEILLE) {
    if (lecture == LOW && maintenant - dernierChangement >= DUREE_REVEIL) {
      reveiller();
    }
    return;
  }

  if (lecture == LOW || lecture != etatBouton ||
      etatTir != REPOS || clicsCourts > 0) {
    derniereActivite = maintenant;
  } else if (maintenant - derniereActivite >= DELAI_VEILLE_AUTO) {
    entrerEnVeille();
    return;
  }

  if (etatTir == RECHARGE) {
    unsigned long ecoule = maintenant - debutPhase;
    if (ecoule >= DUREE_RECHARGE) {
      munitions = CAPACITE;
      couperTir();
      analogWrite(pinLED_R, 0);
      analogWrite(pinLED_V, 255);
      changerEtat(CONFIRMATION);
      debutPhase = maintenant;
      tone(pinBUZZER, 1400);
      attendreRelachement = true;
      clicsCourts = 0;
    } else if (maintenant - dernierEffet >= 20) {
      dernierEffet = maintenant;
      int progression = ecoule * 255UL / DUREE_RECHARGE;
      analogWrite(pinLED_R, 255 - progression);
      analogWrite(pinLED_V, VERT_ORANGE + progression * (255 - VERT_ORANGE) / 255);
      if (ecoule % 400 < 60) {
        unsigned int frequence = static_cast<unsigned int>(700 + ecoule / 2);
        tone(pinBUZZER, frequence);
      } else {
        noTone(pinBUZZER);
      }
    }
    return;
  }

  if (etatTir == CONFIRMATION) {
    unsigned long ecoule = maintenant - debutPhase;
    if (ecoule >= DUREE_BIP_PRET) {
      noTone(pinBUZZER);
    }
    if (ecoule >= DUREE_CONFIRMATION) {
      changerEtat(REPOS);
      dernierPourcentage = 101;
    }
    return;
  }

  if (attendreRelachement) {
    if (lecture == HIGH && maintenant - dernierChangement >= 30) {
      attendreRelachement = false;
      etatBouton = HIGH;
      feuAffiche = false;
      dernierPourcentage = 101;
    }
    return;
  }

  if (lecture == HIGH) {
    digitalWrite(pinLASER, ETEIND_LASER);
    digitalWrite(pinLED, ETEIND_LED);
  }

  if (maintenant - dernierChangement < ANTI_REBOND) return;

  if (etatTir == REPOS && clicsCourts > 0 &&
      maintenant - dernierRelachement > DELAI_DOUBLE_CLIC &&
      (lecture == HIGH || dernierChangement - dernierRelachement > DELAI_DOUBLE_CLIC)) {
    int nombreClics = clicsCourts;
    clicsCourts = 0;
    feuAffiche = false;
    dernierPourcentage = 101;
    if (nombreClics == 3) {
      changerEtat(RECHARGE);
      debutPhase = maintenant;
      dernierEffet = maintenant - 20;
      afficherChargeurVide();
      return;
    }
    if (nombreClics == 1 || nombreClics == 2) {
      verdictDrip = nombreClics == 1;
      randomSeed(micros());
      indexVerdict = random(0, verdictDrip ? 4 : 3);
      debutVerdict = maintenant;
      verdictAffiche = true;
      lancerSonVerdict(maintenant);
    }
  }

  if (lecture != etatBouton) {
    etatBouton = lecture;
    if (lecture == HIGH) {
      bool clicCourt = etatTir == ATTENTE_CLIC &&
                       dernierChangement - debutAppui <= DUREE_CLIC_COURT;
      clicsCourts = clicCourt ? clicsCourts + 1 : 0;
      dernierRelachement = dernierChangement;
      couperTir();
      if (munitions == 0) afficherChargeurVide();
      else {
        analogWrite(pinLED_R, 0);
        analogWrite(pinLED_V, 255);
      }
      changerEtat(REPOS);
      dernierPourcentage = 101;
      if (clicsCourts == 4) entrerEnVeille();
      return;
    }

    if (dernierChangement - dernierRelachement > DELAI_DOUBLE_CLIC) {
      clicsCourts = 0;
    }
    verdictAffiche = false;
    if (sonVerdictActif) {
      noTone(pinBUZZER);
      sonVerdictActif = false;
    }
    changerEtat(ATTENTE_CLIC);
    systemeActive = true;
    debutAppui = dernierChangement;
    debutCharge = maintenant;
    dernierEffet = maintenant;
    debutPulsation = maintenant;
  }

  if (lecture == HIGH) return;

  if (etatTir == ATTENTE_CLIC) {
    if (maintenant - debutAppui > DUREE_CLIC_COURT) {
      if (clicsCourts >= 2) {
        clicsCourts = 0;
        couperTir();
        changerEtat(REPOS);
        dernierPourcentage = 101;
        attendreRelachement = true;
        return;
      }
      bool automatique = clicsCourts == 1;
      clicsCourts = 0;
      if (munitions == 0) {
        signalerVide();
      } else if (automatique) {
        commencerRafale(maintenant);
      } else {
        changerEtat(CHARGEMENT);
        debutCharge = maintenant;
        debutPulsation = maintenant;
        tone(pinBUZZER, FREQUENCE_CHARGE_DEBUT);
      }
    }
    return;
  }

  if (etatTir == TIR_AUTO) {
    if (maintenant - dernierEffet >= 5) {
      dernierEffet = maintenant;
      unsigned long ecoule = maintenant - debutPhase;
      unsigned long cycle = ecoule / PERIODE_TIR_AUTO;
      unsigned long phase = ecoule % PERIODE_TIR_AUTO;
      if (munitions == 0 && (cycle != dernierCycleAuto || phase >= DUREE_TIR_AUTO)) {
        signalerVide();
        return;
      }
      if (cycle != dernierCycleAuto) {
        dernierCycleAuto = cycle;
        tirCycleActif = munitions > 0;
        if (tirCycleActif) {
          munitions--;
          feuAffiche = false;
        }
      }
      bool actif = tirCycleActif && phase < DUREE_TIR_AUTO;
      // La rafale envoie des impulsions distinctes aux recepteurs des cibles.
      digitalWrite(pinLASER, actif ? ALLUME_LASER : ETEIND_LASER);
      digitalWrite(pinLED, actif ? ALLUME_LED : ETEIND_LED);
      if (munitions == 0) afficherChargeurVide();
      else {
        analogWrite(pinLED_R, actif ? 255 : 30);
        analogWrite(pinLED_V, 0);
      }
      if (actif) {
        float restant = 1.0f - static_cast<float>(phase) / DUREE_TIR_AUTO;
        unsigned int frequence = static_cast<unsigned int>(
            FREQUENCE_TIR_FIN + (FREQUENCE_TIR_DEBUT - FREQUENCE_TIR_FIN) * restant * restant);
        tone(pinBUZZER, frequence);
      } else {
        noTone(pinBUZZER);
      }
    }
    return;
  }

  if (etatTir == PAUSE_TIR) {
    if (maintenant - debutPhase >= DUREE_PAUSE) {
      if (munitions == 0) {
        signalerVide();
        return;
      }
      munitions--;
      changerEtat(TIR);
      debutPhase = maintenant;
      dernierEffet = maintenant;
      digitalWrite(pinLED, ALLUME_LED);
      digitalWrite(pinLASER, ALLUME_LASER);
      tone(pinBUZZER, FREQUENCE_TIR_DEBUT);
    }
    return;
  }

  if (etatTir == TIR) {
    unsigned long ecoule = maintenant - debutPhase;
    // Apres le son du tir, le laser reste allume jusqu'au relachement.
    digitalWrite(pinLASER, ALLUME_LASER);
    digitalWrite(pinLED, ALLUME_LED);
    if (ecoule >= DUREE_SON_TIR) {
      noTone(pinBUZZER);
    } else if (maintenant - dernierEffet >= 5) {
      dernierEffet = maintenant;
      float restant = 1.0f - static_cast<float>(ecoule) / DUREE_SON_TIR;
      unsigned int frequence = static_cast<unsigned int>(
          FREQUENCE_TIR_FIN + (FREQUENCE_TIR_DEBUT - FREQUENCE_TIR_FIN) * restant * restant);
      tone(pinBUZZER, frequence);
    }
    return;
  }

  if (etatTir != CHARGEMENT) return;

  unsigned long ecoule = maintenant - debutCharge;
  if (ecoule >= DUREE_CHARGE) {
    noTone(pinBUZZER);
    analogWrite(pinLED_R, 255);
    analogWrite(pinLED_V, 0);
    etatTir = PAUSE_TIR;
    debutPhase = maintenant;
    dernierPourcentage = -1;
  } else if (maintenant - dernierEffet >= 10) {
    dernierEffet = maintenant;
    int progression = ecoule * 255UL / DUREE_CHARGE;
    unsigned long periode = 360 - ecoule * 290UL / DUREE_CHARGE;
    if (maintenant - debutPulsation >= periode) {
      debutPulsation = maintenant;
    }
    bool impulsion = maintenant - debutPulsation < periode / 2;
    int luminosite = impulsion ? 255 : 45;
    analogWrite(pinLED_R, progression * luminosite / 255);
    analogWrite(pinLED_V, (255 - progression) * luminosite / 255);
    float avancement = static_cast<float>(ecoule) / DUREE_CHARGE;
    unsigned int frequence = static_cast<unsigned int>(
        FREQUENCE_CHARGE_DEBUT +
        (FREQUENCE_CHARGE_FIN - FREQUENCE_CHARGE_DEBUT) * avancement * avancement);
    tone(pinBUZZER, frequence);
  }
}

float mesurerDistance() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG, LOW);

  unsigned long duree = pulseIn(ECHO, HIGH, 30000);
  actualiserCharge();

  if (duree == 0) {
    return -1;
  }

  return duree * 0.0343 / 2;
}

void afficherChargement() {
  unsigned long maintenant = millis();
  if (dernierPourcentage >= 0 && maintenant - dernierAffichageCharge < 100) {
    return;
  }
  dernierAffichageCharge = maintenant;

  bool recharge = etatTir == RECHARGE;
  unsigned long duree = recharge ? DUREE_RECHARGE : DUREE_CHARGE;
  unsigned long ecoule = maintenant - (recharge ? debutPhase : debutCharge);
  int pourcentage = ecoule >= duree ? 100 : ecoule * 100UL / duree;
  if (pourcentage == dernierPourcentage) {
    return;
  }

  if (dernierPourcentage < 0) {
    lcd.setCursor(0, 0);
    lcd.print(recharge ? "RECHARGE        " : "CHARGEMENT      ");
  }
  dernierPourcentage = pourcentage;

  lcd.setCursor(0, 1);
  lcd.print("[");
  for (int i = 0; i < 10; i++) {
    lcd.print(i < pourcentage / 10 ? "#" : " ");
  }
  lcd.print("]");
  if (pourcentage < 100) lcd.print(" ");
  if (pourcentage < 10) lcd.print(" ");
  lcd.print(pourcentage);
  lcd.print("%");
}

void afficherDistance() {

  float distance = mesurerDistance();
  if (etatTir != REPOS || munitions == 0 || clicsCourts > 0) {
    return;
  }
  dernierPourcentage = -1;

  lcd.setCursor(0, 0);
  lcd.print("DISTANCE   ");
  if (munitions < 10) lcd.print(" ");
  lcd.print(munitions);
  lcd.print("/20");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  if (distance >= 0) {

    lcd.setCursor(0, 1);
    lcd.print(distance, 1);
    lcd.print(" cm");

  } else {

    lcd.setCursor(0, 1);
    lcd.print("AUCUNE MESURE");
  }

}

void loop() {
  actualiserCharge();

  if (etatTir == VEILLE) {
    delay(5);
    return;
  }

  if (etatTir == CHARGEMENT || etatTir == PAUSE_TIR || etatTir == RECHARGE) {
    afficherChargement();
  } else if (etatTir == CONFIRMATION) {
    if (!feuAffiche) {
      lcd.setCursor(0, 0);
      lcd.print("PRET ! ");
      lcd.print(munitions);
      lcd.print("/");
      lcd.print(CAPACITE);
      lcd.print("    ");
      lcd.setCursor(0, 1);
      lcd.print("RECHARGE FINIE  ");
      feuAffiche = true;
    }
  } else if (etatTir == TIR || etatTir == TIR_AUTO) {
    if (!feuAffiche) {
      lcd.setCursor(0, 0);
      lcd.print(etatTir == TIR_AUTO ? "  MODE AUTO     " : "  >>> FEU ! <<< ");
      lcd.setCursor(0, 1);
      lcd.print("MUNITIONS ");
      if (munitions < 10) lcd.print(" ");
      lcd.print(munitions);
      lcd.print("/20 ");
      feuAffiche = true;
    }
  } else if (etatTir == REPOS && attendreRelachement && digitalRead(pinBTN) == LOW) {
    if (!feuAffiche) {
      lcd.setCursor(0, 0);
      lcd.print("RELACHEZ BOUTON ");
      lcd.setCursor(0, 1);
      lcd.print("POUR CONTINUER  ");
      feuAffiche = true;
    }
  } else if (etatTir == REPOS && verdictAffiche) {
    if (!feuAffiche) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(verdictDrip ? "VERDICT: DRIP!" : "VERDICT PAS DRIP");
      lcd.setCursor(0, 1);
      lcd.print(verdictDrip ? MESSAGES_DRIP[indexVerdict]
                            : MESSAGES_PAS_DRIP[indexVerdict]);
      feuAffiche = true;
    }
  } else if (etatTir == ATTENTE_CLIC || (etatTir == REPOS && clicsCourts > 0)) {
    if (!feuAffiche) {
      lcd.setCursor(0, 0);
      lcd.print("CLICS : ");
      lcd.print(clicsCourts);
      lcd.print("       ");
      lcd.setCursor(0, 1);
      if (clicsCourts == 0) lcd.print("MAINTENIR:CHARGE");
      else if (clicsCourts == 1) lcd.print("MAINTENIR: AUTO ");
      else if (clicsCourts == 2) lcd.print("3 CLICS:RECHARGE");
      else lcd.print("4 CLICS: VEILLE ");
      feuAffiche = true;
    }
  } else if (etatTir == VIDE ||
             (etatTir == REPOS && munitions == 0)) {
    if (!feuAffiche) {
      lcd.setCursor(0, 0);
      lcd.print(munitions == 0 ? "CHARGEUR VIDE   " : "MAINTENIR: AUTO ");
      lcd.setCursor(0, 1);
      lcd.print("3 CLICS:RECHARGE");
      feuAffiche = true;
    }
  } else if (systemeActive && digitalRead(pinBTN) == HIGH &&
             (dernierPourcentage >= 0 || millis() - derniereMesure >= 250)) {
    afficherDistance();
    derniereMesure = millis();
  }

  actualiserCharge();
  delay(1);
}
