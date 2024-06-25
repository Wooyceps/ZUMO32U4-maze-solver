#include <Wire.h>
#include <Zumo32U4.h>

// Inicjalizacja różnych komponentów robota Zumo32U4
Zumo32U4LCD display;
Zumo32U4Buzzer buzzer;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4Motors motors;
Zumo32U4LineSensors lineSensors;
Zumo32U4IMU imu;

#include "TurnSensor.h"
#include "GridMovement.h"

// Deklaracja tablic do przechowywania ścieżki i odwróconej ścieżki
char path[100];
char reversePath[100];
uint8_t pathLength = 0;
uint8_t reversePathLength = 0;

unsigned long start;
unsigned long time;
unsigned long segmentTimes[100];
unsigned long seg_time = 1300; // maksymalny czas na jeden segment

void setup()
{
  // Gra dźwięk na rozpoczęcie
  buzzer.playFromProgramSpace(PSTR("!>g32>>c32"));
  gridMovementSetup();  // Inicjalizacja ruchu po siatce - głównie kalibracja potrzebnych podzespołów
  mazeSolve();  // Rozwiązywanie labiryntu
}

void loop()
{
  computeReversePath();  // Oblicza odwróconą ścieżkę
  buttonA.waitForButton();  // Czeka na naciśnięcie przycisku A
  mazeFollowReversePath();  // Podąża za odwróconą ścieżką
  buttonA.waitForButton();  // Czeka na naciśnięcie przycisku A
  mazeFollowPath();  // Podąża za zarejestrowaną ścieżką
}

char selectTurn(bool foundLeft, bool foundStraight, bool foundRight)
{
  // Wybiera kierunek na podstawie dostępnych opcji
  if(foundLeft&&(!((path[-3]='L')&&(path[-2]='L')&&(path[-1]='L')))) { return 'L'; }  // W lewo
  else if(foundStraight) { return 'S'; }  // Prosto
  else if(foundRight) { return 'R'; }  // W prawo
  else { return 'B'; }  // Powrót
}

void mazeSolve()
{
  pathLength = 0;  // Resetowanie długości ścieżki
  buzzer.playFromProgramSpace(PSTR("!L16 cdegreg4"));  // Dźwięk na rozpoczęcie rozwiązywania labiryntu
  delay(1000);

  while(1)
  {
    start = millis();
    followSegment();  // Podążanie za linią
    segmentTime = millis() - start;
    segmentTimes[pathLenght] = segmentTime;
    bool foundLeft, foundStraight, foundRight;
    driveToIntersectionCenter(&foundLeft, &foundStraight, &foundRight);  // Dojazd do centrum skrzyżowania
    if(aboveDarkSpot())  // Sprawdza, czy jest nad ciemnym punktem (meta)
    {
      break;
    }

    char dir = selectTurn(foundLeft, foundStraight, foundRight);  // Wybór kierunku

    if (pathLength >= sizeof(path))  // Sprawdza, czy tablica ścieżki nie jest pełna - mniejsza niż 100
    {
      display.clear();
      display.print(F("pathfull"));
      while(1)
      {
        ledRed(1);  // Zapala czerwoną diodę
      }
    }

    path[pathLength] = dir;  // Zapisuje kierunek do tablicy ścieżki
    pathLength++; // Obecna ilość podjętych decyzji na skrzyżowaniach
    simplifyPath();  // Upraszcza ścieżkę
    displayPath();  // Wyświetla ścieżkę na wyświetlaczu

    turn(dir);  // Skręca w wybranym kierunku
  }

  motors.setSpeeds(0, 0);  // Zatrzymuje silniki - dotarł na mete
  buzzer.playFromProgramSpace(PSTR("!>>a32"));  // Gra dźwięk na zakończenie
}

void mazeFollowPath()
{
  buzzer.playFromProgramSpace(PSTR("!>c32"));  // Gra dźwięk na rozpoczęcie
  delay(1000);

  for(uint16_t i = 0; i < pathLength; i++)
  {
    followSegment();  // Podążanie za segmentem linii
    driveToIntersectionCenter();  // Dojazd do centrum skrzyżowania
    turn(path[i]);  // Skręca w zapisanym kierunku
  }

  followSegment();  // Podążanie za ostatnim segmentem linii
  motors.setSpeeds(0, 0);  // Zatrzymuje silniki
  buzzer.playFromProgramSpace(PSTR("!>>a32"));  // Gra dźwięk na zakończenie
}

void mazeFollowReversePath()
{
  buzzer.playFromProgramSpace(PSTR("!>e32"));  // Gra dźwięk na rozpoczęcie
  delay(1000);

  for(uint16_t i = 0; i < reversePathLength; i++)
  {
    followSegment();  // Podążanie za segmentem linii
    driveToIntersectionCenter();  // Dojazd do centrum skrzyżowania
    turn(reversePath[i]);  // Skręca w zapisanym kierunku (odwróconym)
  }

  followSegment();  // Podążanie za ostatnim segmentem linii
  motors.setSpeeds(0, 0);  // Zatrzymuje silniki
  buzzer.playFromProgramSpace(PSTR("!>>a32"));  // Gra dźwięk na zakończenie
}

void computeReversePath()
{
  reversePathLength = pathLength;  // Ustawia długość odwróconej ścieżki
  for (uint8_t i = 0; i < pathLength; i++)
  {
    char dir = path[pathLength - 1 - i];  // Pobiera ostatni element z tablicy ścieżki
    switch(dir)  // Zamienia kierunki na odwrócone
    {
      case 'L': reversePath[i] = 'R'; break;
      case 'R': reversePath[i] = 'L'; break;
      case 'S': reversePath[i] = 'S'; break;
      case 'B': reversePath[i] = 'B'; break;
    }
  }
  displayReversePath();
}

void simplifyPath()
{
  if(pathLength < 3)
  {
    return;  // Jeśli ścieżka jest za krótka lub przedostatni element NIE jest 'B', nie upraszcza
  }

  

  if(path[pathLenght - 2] != 'B')
  {
    return;
  }

  int16_t totalAngle = 0;

  for(uint8_t i = 1; i <= 3; i++)
  {
    switch(path[pathLength - i])  // Sumuje kąty skrętów
    {
    case 'L':
      totalAngle += 90;
      break;
    case 'R':
      totalAngle -= 90;
      break;
    case 'B':
      totalAngle += 180;
      break;
    }
  }

  totalAngle = totalAngle % 360;  // Normalizuje kąt do zakresu 0-359 stopni

  switch(totalAngle)  // Zastępuje trzy skręty jednym odpowiednim
  {
  case 0:
    path[pathLength - 3] = 'S';
    break;
  case 90:
    path[pathLength - 3] = 'L';
    break;
  case 180:
    path[pathLength - 3] = 'B';
    break;
  case 270:
    path[pathLength - 3] = 'R';
    break;
  }

  pathLength -= 2;  // Aktualizuje długość ścieżki
}
void displayPath()
{
  path[pathLength] = 0;  // Dodaje znak końca stringa
  display.clear();
  display.print(path);  // Wyświetla ścieżkę na wyświetlaczu
  if(pathLength > 8)  // Jeśli ścieżka jest dłuższa niż 8 znaków, wyświetla resztę w nowej linii
  {
    display.gotoXY(0, 1);
    display.print(path + 8);
  }
}
void displayReversePath()
{
  reversePath[pathLength] = 0;  // Dodaje znak końca stringa
  display.clear();
  display.print(reversePath);  // Wyświetla ścieżkę na wyświetlaczu
  if(pathLength > 8)  // Jeśli ścieżka jest dłuższa niż 8 znaków, wyświetla resztę w nowej linii
  {
    display.gotoXY(0, 1);
    display.print(reversePath + 8);
  }
}
