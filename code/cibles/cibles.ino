// =========================
// CAPTEURS LUMINEUX
// =========================
#define CAPTEUR_1 5   // D1
#define CAPTEUR_2 4   // D2

// =========================
// RGB 1 - PERSONNAGE 1
// =========================
#define RGB1_G 14      // D5
#define RGB1_R 12      // D6
#define RGB1_B 13      // D7

// =========================
// RGB 2 - PERSONNAGE 2
// =========================
#define RGB2_G 16      // D0
#define RGB2_R 0       // D3
#define RGB2_B 2       // D4

#define BUZZER 15      // D8

bool mort1 = false;
bool mort2 = false;
bool pret1 = false;
bool pret2 = false;
unsigned long touche1A = 0;
unsigned long touche2A = 0;

const unsigned long DELAI_REINITIALISATION = 5000; // 5 secondes
// Son de KO arcade : attaque, tremblement, puis chute grave.
// Une frequence de 0 correspond a une courte pause.
const int NOTES_KO[] = {1500, 1050, 700, 0, 560, 750, 500, 680, 420, 320, 240, 180};
const unsigned long DUREES_KO[] = {45, 55, 80, 35, 90, 75, 90, 75, 110, 140, 180, 240};
const int NOMBRE_NOTES = sizeof(NOTES_KO) / sizeof(NOTES_KO[0]);
const int NIVEAU_LASER = LOW; // Le recepteur passe a LOW quand il est eclaire
const unsigned long INTERVALLE_AFFICHAGE = 200;
unsigned long dernierAffichage = 0;
unsigned long debutNote = 0;
int noteActuelle = 0;
bool buzzerActif = false;


// =========================
// COULEUR RGB
// =========================
void vertRGB(int G, int R, int B) {
  digitalWrite(G, HIGH);
  digitalWrite(R, LOW);
  digitalWrite(B, LOW);
}

void rougeRGB(int G, int R, int B) {
  digitalWrite(G, LOW);
  digitalWrite(R, HIGH);
  digitalWrite(B, LOW);
}

void sonner(unsigned long maintenant) {
  noteActuelle = 0;
  debutNote = maintenant;
  buzzerActif = true;
  tone(BUZZER, NOTES_KO[noteActuelle]);
}

void mettreAJourSon(unsigned long maintenant) {
  if (!buzzerActif || maintenant - debutNote < DUREES_KO[noteActuelle]) {
    return;
  }

  debutNote = maintenant;
  noteActuelle++;
  if (noteActuelle >= NOMBRE_NOTES) {
    noTone(BUZZER);
    buzzerActif = false;
  } else if (NOTES_KO[noteActuelle] == 0) {
    noTone(BUZZER);
  } else {
    tone(BUZZER, NOTES_KO[noteActuelle]);
  }
}

void setup() {

  Serial.begin(115200);

  // Capteurs
  pinMode(CAPTEUR_1, INPUT);
  pinMode(CAPTEUR_2, INPUT);

  // RGB 1
  pinMode(RGB1_G, OUTPUT);
  pinMode(RGB1_R, OUTPUT);
  pinMode(RGB1_B, OUTPUT);

  // RGB 2
  pinMode(RGB2_G, OUTPUT);
  pinMode(RGB2_R, OUTPUT);
  pinMode(RGB2_B, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Les deux personnages commencent VIVANTS
  vertRGB(RGB1_G, RGB1_R, RGB1_B);
  vertRGB(RGB2_G, RGB2_R, RGB2_B);

  Serial.println("================================");
  Serial.println("  SYSTEME DES PERSONNAGES");
  Serial.println("================================");
  Serial.println("Personnage 1 : VIVANT");
  Serial.println("Personnage 2 : VIVANT");
  Serial.println("Attente d'un etat sans laser sur chaque capteur");
}


void loop() {

  int capteur1 = digitalRead(CAPTEUR_1);
  int capteur2 = digitalRead(CAPTEUR_2);
  unsigned long maintenant = millis();

  mettreAJourSon(maintenant);

  if (maintenant - dernierAffichage >= INTERVALLE_AFFICHAGE) {
    dernierAffichage = maintenant;
    Serial.print("Capteur 1 = ");
    Serial.print(capteur1);
    Serial.print(" | Capteur 2 = ");
    Serial.println(capteur2);
  }


  // =========================
  // PERSONNAGE 1
  // =========================

  if (mort1 && maintenant - touche1A >= DELAI_REINITIALISATION) {
    mort1 = false;
    pret1 = false;
    vertRGB(RGB1_G, RGB1_R, RGB1_B);
    Serial.println("Personnage 1 : REINITIALISE");
  }

  if (!mort1 && capteur1 != NIVEAU_LASER) {
    pret1 = true;
  }

  if (!mort1 && pret1 && capteur1 == NIVEAU_LASER) {

    mort1 = true;
    pret1 = false;
    touche1A = maintenant;

    rougeRGB(RGB1_G, RGB1_R, RGB1_B);
    sonner(maintenant);

    Serial.println("💥 PERSONNAGE 1 TOUCHE !");
  }


  // =========================
  // PERSONNAGE 2
  // =========================

  if (mort2 && maintenant - touche2A >= DELAI_REINITIALISATION) {
    mort2 = false;
    pret2 = false;
    vertRGB(RGB2_G, RGB2_R, RGB2_B);
    Serial.println("Personnage 2 : REINITIALISE");
  }

  if (!mort2 && capteur2 != NIVEAU_LASER) {
    pret2 = true;
  }

  if (!mort2 && pret2 && capteur2 == NIVEAU_LASER) {

    mort2 = true;
    pret2 = false;
    touche2A = maintenant;

    rougeRGB(RGB2_G, RGB2_R, RGB2_B);
    sonner(maintenant);

    Serial.println("💥 PERSONNAGE 2 TOUCHE !");
  }

  delay(5);
}
