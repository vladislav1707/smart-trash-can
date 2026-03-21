// идеи для улучшения:
// 1. режим энергосбережения когда долго не используется
// 2. изменение настроек через терминал и их сохранение в EEPROM

#include <Servo.h>
#include <EEPROM.h>

// пины
#define SERVO_PIN 3
#define TRIG_PIN 4
#define ECHO_PIN 5

// первый запуск
#define INIT_ADDR 1023  // номер резервной ячейки
#define INIT_KEY 67     // ключ первого запуска

// настройки(заводские)
struct Settings {
  int ACTIVATION_HEIGHT = 25;     // высота срабатывания   (сантиметры)
  int CAP_TIME = 5000;            // время открытой крышки (миллисекунды)
  int OPEN_ANGLE = 0;             // угол открытия крышки  (градусы)
  int CLOSE_ANGLE = 150;          // угол закрытия крышки  (градусы)
  int LID_OPEN_TIME = 800;        // время открытия крышки (миллисекунды)
  int LID_CLOSE_TIME = 900;       // время закрытия крышки (миллисекунды)
};

Settings settings;

int debugMode = 0;              // режим отладки

// сервопривод
Servo LidServo;

int state = 1; // состояние
double tmr1 = millis(); // таймер 1
double tmr2 = millis(); // таймер 2
double sensorDistance;

void processCommand(String command)
{ 
  if (command == "help")
  {
    Serial.println("\n=== AVAILABLE COMMANDS ===");
    Serial.println("help           - Show this help");
    Serial.println("debug          - toggle debug");
    Serial.println("settings       - Show current settings");
    Serial.println("open           - Manually open lid");
    Serial.println("close          - Manually close lid");
    Serial.println("auto           - automatic lid control");
    Serial.println("version        - print version");
    Serial.println("set            - change settings"); // не доделано
    Serial.println("==========================");
  }
  else if (command == "settings")
  {
    Serial.println("\n=== CURRENT SETTINGS===");
    Serial.print("ACTIVATION_HEIGHT: "); Serial.println(settings.ACTIVATION_HEIGHT);
    Serial.print("CAP_TIME: "); Serial.println(settings.CAP_TIME);
    Serial.print("OPEN_ANGLE: "); Serial.println(settings.OPEN_ANGLE);
    Serial.print("CLOSE_ANGLE: "); Serial.println(settings.CLOSE_ANGLE);
    Serial.print("LID_OPEN_TIME: "); Serial.println(settings.LID_OPEN_TIME);
    Serial.print("LID_CLOSE_TIME: "); Serial.println(settings.LID_CLOSE_TIME);
    Serial.println("==========================");
  }
  else if (command == "debug")
  {
    debugMode = !debugMode;
  }
  else if (command == "version")
  {
    Serial.println("\nversion: 1.0.1");
  }
  else if (command == "close") // принудительно закрыть
  {
    LidServo.write(settings.CLOSE_ANGLE);
    state = 5;
    Serial.println("Lid manually closed. Send 'auto' to return to automatic mode.");
  }
  else if (command == "open")  // принудительно открыть
  {
    LidServo.write(settings.OPEN_ANGLE);
    tmr1 = millis();           // перед переходом в состояние 3 обновляем таймер 1
    state = 6;
    Serial.println("Lid manualy opened. Send 'auto' to return to automatic mode.");
  }
  else if (command == "auto")  // переход в автоматический режим
  {
    if(state == 5)             // если принудительно закрыто перейти в состояние 1(закрыто)
    {
      state = 1;
    }
    else if(state == 6)        // если принудительно открыто перейти в состояние 3(открыто)
    {
      state = 3;
    }
    else
    {
      Serial.println("Already in automatic mode.");
    }
  }
  else if (command == "set")
  {
    // TODO: set арг1 арг2, где арг1 настройка значение которой будет изменено, а арг2 это новое значение
  }
  else
  {
    Serial.println("Command not found. Type 'help' to see a list of commands");
  }
}

double measure() {
  // импульс 10 микросекунд
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // измерение времени ответного импульса
  int duration = pulseIn(ECHO_PIN, HIGH);

  // считаем расстояние и возвращаем
  return duration / 58.2;
}

void setup() {
  Serial.begin(9600);

  Serial.println("programm started");

  if (EEPROM.read(INIT_ADDR) != INIT_KEY) { // если первый запуск
    EEPROM.write(INIT_ADDR, INIT_KEY);      // записать ключ в яйчейку ключа

    // в случае первого запуска(или сброса до заводских) загрузить заводские настройки
    EEPROM.put(0, settings);
  }
  else // если запуск все же не первый подгрузим настройки из EEPROM
  {
    EEPROM.get(0, settings); // читать настройки
  }

  // сенсор
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  LidServo.attach(SERVO_PIN);
  LidServo.write(settings.CLOSE_ANGLE); // изначально при запуске программы крышка закрывается
}

void loop() {
  sensorDistance = measure();

  if (Serial.available() > 0) {                     // если доступна комманда обрабатываем
    String command = Serial.readStringUntil('\n');  // читаем команду до \n (перенос строки)
    command.toLowerCase();                          // приводим к нижнему регистру
    command.trim();                                 // Удаляем ЛИШНИЕ пробелы и символы переноса строки
    processCommand(command);                        // функция обработки
  }

  if (debugMode)
  {
    Serial.print("state: ");
    Serial.print(state);
    Serial.print("\t");
    Serial.print("sensorDistance: ");
    Serial.print(sensorDistance);
    Serial.print("\t");
    Serial.print("mode: ");
    if(state == 5 or state == 6)
    {
      Serial.println("manual");
    }
    else
    {
      Serial.println("auto");
    }
  }

switch (state) {
  case 1:                   //закрыто, когда подносят руку переходит в состояние 2(открывается)
    if(sensorDistance <= settings.ACTIVATION_HEIGHT)
    {
      LidServo.write(settings.OPEN_ANGLE);
      state = 2;            // переходим в состояние 2(открывается)
    }
    break;
  case 2:                   // открывается, ждет и переходит в состояние 3(открыто)
    delay(settings.LID_OPEN_TIME);   // ждем
    tmr1 = millis();        // перед переходом в состояние 3 обновляем таймер 1
    state = 3;              // переходим в состояние 3(открыто)
    break;
  case 3:                   // открыто, ожидает определенное время и переходит в состояние 4(закрывается), обнаружение руки обновляет таймер 1
    {
      if(millis() - tmr1 >= settings.CAP_TIME)
      {
        LidServo.write(settings.CLOSE_ANGLE);
        tmr2 = millis();    // таймер 2 обновляется
        state = 4;          // ждем, и переходим с состояние 4(закрывается)
      }
      if(sensorDistance <= settings.ACTIVATION_HEIGHT)
      {
        tmr1 = millis();    // таймер 1 обновляется
      }
    }
    break;
  case 4:                   // закрывается, при обнаружении руки переходит в состоянияе 2(открывается). Со временем перейдет в состояние 1(закрыто) если не обнаружит руку
    {
      if(sensorDistance <= settings.ACTIVATION_HEIGHT)
      {
        LidServo.write(settings.OPEN_ANGLE);
        state = 2;          // переход в состояние 2(открывается)
      }
      else if(millis() - tmr2 >= settings.LID_CLOSE_TIME)
      {
        state = 1;          // переход в состояние 1(закрыто)
      }
    }
    break;
    case 5:                 // принудительное закрытие
      // отключается и включается командой
      break;
    case 6:                 // принудительное открытие
      // отключается и включается командой
      break;
}
delay(100);
}