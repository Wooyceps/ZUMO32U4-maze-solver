// GridMovement.h zapewnia funkcje, które pomagają Zumo poruszać się po siatce
// złożonej z czarnych linii na białej powierzchni.
//
// Kod używa czujników linii do śledzenia linii i wykrywania
// skrzyżowań, a także używa żyroskopu do wykonywania skrętów.
//
// Kod ten został zaprojektowany z myślą o rozwiązywaniu labiryntów, ale może być
// używany w innych zastosowaniach, o ile są czarne linie na białej powierzchni,
// a linie nie są zbyt blisko siebie i przecinają się pod kątem prostym.
//
// Prędkości i opóźnienia w tym pliku zostały zaprojektowane dla
// Zumo 32U4 z 4 bateriami NiMH i silnikami 75:1 HP.
// Mogą wymagać dostosowania w zależności od silnika i napięcia baterii.
//
// Ten plik powinien być dołączony raz do skeczu, gdzieś
// po dołączeniu TurnSensor.h i zdefiniowaniu obiektów nazwanych
// buttonA, display, imu, lineSensors i motors.

#pragma once

#include <Wire.h>

// Prędkość silników przy jeździe prosto. 400 to maksymalna prędkość.
const uint16_t straightSpeed = 200;

// Opóźnienie pomiędzy wykryciem skrzyżowania a
// rozpoczęciem skrętu. W tym czasie robot jedzie
// prosto. Idealnie, to opóźnienie powinno być na tyle długie, aby
// robot dotarł z punktu wykrycia skrzyżowania do jego środka.
const uint16_t intersectionDelay = 130;

// Prędkość silników przy skręcaniu. 400 to maksymalna prędkość.
const uint16_t turnSpeed = 200;

// Prędkość silników przy kalibracji czujników linii.
const uint16_t calibrationSpeed = 200;

// Próg czujnika linii używany do wykrywania skrzyżowań.
const uint16_t sensorThreshold = 200;

// Próg czujnika linii używany do wykrywania końca
// labiryntu.
const uint16_t sensorThresholdDark = 600;

// Liczba używanych czujników linii.
const uint8_t numSensors = 5;

// Dla kątów mierzonych przez żyroskop, nasza konwencja jest taka, że
// wartość (1 << 29) reprezentuje 45 stopni. To oznacza, że
// uint32_t może reprezentować dowolny kąt pomiędzy 0 a 360.
const int32_t gyroAngle45 = 0x20000000;

uint16_t lineSensorValues[numSensors];

// Ustawia specjalne znaki dla wyświetlacza, aby można było pokazać
// wykres słupkowy.
static void loadCustomCharacters()
{
  static const char levels[] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 63, 63, 63, 63, 63, 63, 63
  };
  display.loadCustomCharacter(levels + 0, 0);  // 1 słupek
  display.loadCustomCharacter(levels + 1, 1);  // 2 słupki
  display.loadCustomCharacter(levels + 2, 2);  // 3 słupki
  display.loadCustomCharacter(levels + 3, 3);  // 4 słupki
  display.loadCustomCharacter(levels + 4, 4);  // 5 słupków
  display.loadCustomCharacter(levels + 5, 5);  // 6 słupków
  display.loadCustomCharacter(levels + 6, 6);  // 7 słupków
}

void printBar(uint8_t height)
{
  if (height > 8) { height = 8; }
  const char barChars[] = {' ', 0, 1, 2, 3, 4, 5, 6, (char)255};
  display.print(barChars[height]);
}

// Wykonuje skalibrowane odczyty czujników linii i zapisuje je
// w lineSensorValues. Zwraca również oszacowanie pozycji linii.
uint16_t readSensors()
{
  return lineSensors.readLine(lineSensorValues);
}

// Zwraca true, jeśli czujnik widzi linię.
// Upewnij się, że wywołałeś readSensors() przed wywołaniem tej funkcji.
bool aboveLine(uint8_t sensorIndex)
{
  return lineSensorValues[sensorIndex] > sensorThreshold;
}

// Zwraca true, jeśli czujnik widzi dużo ciemności.
// Upewnij się, że wywołałeś readSensors() przed wywołaniem tej funkcji.
bool aboveLineDark(uint8_t sensorIndex)
{
  return lineSensorValues[sensorIndex] > sensorThresholdDark;
}

// Sprawdza, czy jesteśmy nad ciemnym miejscem, takim jak te używane
// do oznaczania końca labiryntu. Jeśli wszystkie środkowe czujniki są nad
// ciemnością, oznacza to, że znaleźliśmy to miejsce.
// Upewnij się, że wywołałeś readSensors() przed wywołaniem tej funkcji.
bool aboveDarkSpot()
{
  return aboveLineDark(1) && aboveLineDark(2) && aboveLineDark(3);
}

// Skręca zgodnie z parametrem dir, który powinien być 'L'
// (lewo), 'R' (prawo), 'S' (prosto) lub 'B' (wstecz). Skręcamy
// większość drogi używając żyroskopu, a następnie używamy jednego z czujników linii
// do zakończenia skrętu. Używamy wewnętrznego czujnika linii, który
// jest bliżej docelowej linii, aby zmniejszyć nadmierny skręt.
void turn(char dir)
{
  if (dir == 'S')
  {
    // Nie rób nic!
    return;
  }

  turnSensorReset();

  uint8_t sensorIndex;

  switch(dir)
  {
  case 'B':
    // Skręć w lewo o 135 stopni używając żyroskopu.
    motors.setSpeeds(-turnSpeed, turnSpeed);
    while((int32_t)turnAngle < turnAngle45 * 3)
    {
      turnSensorUpdate();
    }
    sensorIndex = 1;
    break;

  case 'L':
    // Skręć w lewo o 45 stopni używając żyroskopu.
    motors.setSpeeds(-turnSpeed, turnSpeed);
    while((int32_t)turnAngle < turnAngle45)
    {
      turnSensorUpdate();
    }
    sensorIndex = 1;
    break;

  case 'R':
    // Skręć w prawo o 45 stopni używając żyroskopu.
    motors.setSpeeds(turnSpeed, -turnSpeed);
    while((int32_t)turnAngle > -turnAngle45)
    {
      turnSensorUpdate();
    }
    sensorIndex = 3;
    break;

  default:
    // To nie powinno się zdarzyć.
    return;
  }

  // Skręć resztę drogi używając czujników linii.
  while(1)
  {
    readSensors();
    if (aboveLine(sensorIndex))
    {
      // Znaleźliśmy linię ponownie, więc skręt jest zakończony.
      break;
    }
  }
}

void followSegment() // jedzie prosto po linii aż znajdzie skrzyżowanie, ślepą uliczkę albo ciemne miejsce
{
  while(1)
  {
    // Uzyskaj pozycję linii.
    uint16_t position = readSensors();

    // Nasz "błąd" to odległość od środka linii,
    // co odpowiada pozycji 2000.
    int16_t error = (int16_t)position - 2000;

    // Oblicz różnicę pomiędzy prędkościami dwóch silników,
    // leftSpeed - rightSpeed.
    int16_t speedDifference = error / 4;

    // Uzyskaj indywidualne prędkości silników. Znak speedDifference
    // określa, czy robot skręca w lewo czy w prawo.
    int16_t leftSpeed = (int16_t)straightSpeed + speedDifference;
    int16_t rightSpeed = (int16_t)straightSpeed - speedDifference;

    // Ogranicz prędkości silników do zakresu 0 do straightSpeed.
    leftSpeed = constrain(leftSpeed, 0, (int16_t)straightSpeed);
    rightSpeed = constrain(rightSpeed, 0, (int16_t)straightSpeed);

    motors.setSpeeds(leftSpeed, rightSpeed);

    // Używamy wewnętrznych czterech czujników (1, 2, 3 i 4) do
    // określenia, czy jest linia prosto przed nami, oraz
    // czujników 0 i 5 do wykrywania linii idących w lewo i
    // prawo.
    //
    // Ten kod można by poprawić, pomijając poniższe sprawdzenia,
    // jeśli od początku tej funkcji minęło mniej niż 200 ms.
    // Rozwiązania labiryntów czasem kończą się w złej pozycji
    // po skręcie, i jeśli jeden z dalekich czujników jest nad
    // linią, może to spowodować fałszywe wykrycie skrzyżowania.

    if(!aboveLine(0) && !aboveLine(1) && !aboveLine(2) && !aboveLine(3) && !aboveLine(4))
    {
      // Nie ma widocznej linii przed nami i nie widzieliśmy
      // skrzyżowania. Musi to być ślepa uliczka.
      break;
    }

    if(aboveLine(0) || aboveLine(4))
    {
      // Znaleziono skrzyżowanie lub ciemne miejsce.
      break;
    }
  }
}

// To powinno być wywołane po followSegment, aby dojechać do
// środka skrzyżowania.
void driveToIntersectionCenter()
{
  // Jedź do środka skrzyżowania.
  motors.setSpeeds(straightSpeed, straightSpeed);
  delay(intersectionDelay);
}

// To powinno być wywołane po followSegment, aby dojechać do
// środka skrzyżowania. Używa także czujników linii do
// wykrywania wyjść w lewo, prosto i w prawo.
void driveToIntersectionCenter(bool * foundLeft, bool * foundStraight, bool * foundRight)
{
  *foundLeft = 0;
  *foundStraight = 0;
  *foundRight = 0;

  // Jedź prosto do środka
  // skrzyżowania, jednocześnie sprawdzając wyjścia w lewo i
  // w prawo.
  //
  // readSensors() zajmuje około 2 ms, więc używamy
  // tego do timing pętli. Bardziej niezawodne podejście to
  // użycie millis() do timing.
  motors.setSpeeds(straightSpeed, straightSpeed);
  for(uint16_t i = 0; i < intersectionDelay / 2; i++)
  {
    readSensors();
    if(aboveLine(0))
    {
      *foundLeft = 1;
    }
    if(aboveLine(4))
    {
      *foundRight = 1;
    }
  }

  readSensors();

  // Sprawdź wyjście prosto.
  if(aboveLine(1) || aboveLine(2) || aboveLine(3))
  {
    *foundStraight = 1;
  }
}

// Kalibruje czujniki linii, obracając się w lewo i w prawo, a następnie
// wyświetla wykres słupkowy skalibrowanych odczytów czujników na wyświetlaczu.
// Zwraca po naciśnięciu przez użytkownika przycisku A.
static void lineSensorSetup()
{
  display.clear();
  display.print(F("Line cal"));

  // Opóźnienie, aby robot nie poruszał się, gdy użytkownik jeszcze
  // dotyka przycisku.
  delay(1000);

  // Używamy żyroskopu do skręcania, aby nie skręcać więcej niż
  // to konieczne, i aby w razie problemów z żyroskopem
  // wiedzieć przed faktycznym uruchomieniem robota.

  turnSensorReset();

  // Skręć w lewo o 90 stopni.
  motors.setSpeeds(-calibrationSpeed, calibrationSpeed);
  while((int32_t)turnAngle < turnAngle45 * 2)
  {
    lineSensors.calibrate();
    turnSensorUpdate();
  }

  // Skręć w prawo o 90 stopni.
  motors.setSpeeds(calibrationSpeed, -calibrationSpeed);
  while((int32_t)turnAngle > -turnAngle45 * 2)
  {
    lineSensors.calibrate();
    turnSensorUpdate();
  }

  // Skręć z powrotem do środka używając żyroskopu.
  motors.setSpeeds(-calibrationSpeed, calibrationSpeed);
  while((int32_t)turnAngle < 0)
  {
    lineSensors.calibrate();
    turnSensorUpdate();
  }

  // Zatrzymaj silniki.
  motors.setSpeeds(0, 0);

  // Wyświetl odczyty czujników linii na wyświetlaczu, aż zostanie
  // naciśnięty przycisk A.
  display.clear();
  while(!buttonA.getSingleDebouncedPress())
  {
    readSensors();

    display.gotoXY(0, 0);
    for (uint8_t i = 0; i < numSensors; i++)
    {
      uint8_t barHeight = map(lineSensorValues[i], 0, 1000, 0, 8);
      printBar(barHeight);
    }
  }

  display.clear();
}

void gridMovementSetup()
{
  // Skonfiguruj piny używane dla czujników linii.
  lineSensors.initFiveSensors();

  // Ustaw specjalne znaki na wyświetlaczu, aby można było pokazać wykres
  // słupkowy odczytów czujników po kalibracji.
  loadCustomCharacters();

  // Skalibruj żyroskop i pokaż jego odczyty, aż użytkownik
  // naciśnie przycisk A.
  turnSensorSetup();

  // Skalibruj czujniki, obracając się w lewo i w prawo, i pokaż
  // odczyty, aż użytkownik ponownie naciśnie przycisk A.
  lineSensorSetup();
}