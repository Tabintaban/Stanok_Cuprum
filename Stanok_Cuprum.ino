//Скетч под три кнопки управления станком по намотке провода от Proektirovshik
//удален oled дисплей
//видео станка ниже на ютубе и дзене
// https://youtu.be/01Un0Ifq334
// https://zen.yandex.ru/video/watch/626ea9b95bdcdb7bacf09e1d
//управляет сервоприводом укладчика провода и основным мотрчиком с редуктором
//с кнопок на выводах 11 больше и 12 меньше устанавливается значение количества витков и нажимается пуск на кнопке вывод 10
//моторредуктор висит на 3 и 4 выводах через плату контроллера D293
//сервопривод висит на 9 выводе через плату контроллера D293
//управление экраном по шине I2C

#include <Wire.h>                   //объявление библиотеки Wire
#include <LiquidCrystal_I2C.h>      //объявление библиотеки для вывода текста на LCD1602
#include <Servo.h>                  //объявление библиотеки для сервопривода
Servo servo;                        // Создаем объект сервопривод
LiquidCrystal_I2C lcd(0x27, 16, 2); //установка lcd1602 на адрес 0x27
int UP = 12;                        //кнопка переманной вперед UP присвоено значение 12
int DOWN = 11;                      //кнопка переменной назад DOWN присвоено значение 11
int START = 10;                     //кнопка переменной старт START присвоено значение 10
int NAGREV = 2;                     //порт управления нагревом
int GERKON = 8;                     //порт 8 называем GERKON / оптодатчик
int x = 0;                          //переменной x присвоено значение 0
unsigned short y = 0;               //желаемое количество витков переменной y (0 до 65535) присвоено изначально значение 0
unsigned int CountGerkon = 0;       //счетчик Геркона / оптодачика
static int j = 1;                   //вспомогательной переменной j присвоена 1 для цикла серво привода
float l = 150;                      //крайний левый угол в градусах
float r = 60;                       //крайний правый угол в градусах
float g = l;                        //g - (g=l начинаем намотку слева, g=r начинаем намотку справа )
float q = 2;                        //q - угол смещения каретки на 1 оборот (2 градуса) 
#define MOT1_STP 4                  // MOT1_STP присвоить значение вывода 4
#define MOT1_DIR 3                  // MOT1_DIR присвоить значение вывода 3
unsigned long last_time;

void setup()
{
  servo.attach(9);               // Указываем объекту класса Servo, что серво присоединен к пину 9
  servo.write(g);                // Выставляем начальное положение 0 град. Диапазон (0-180) град.
  Wire.begin();                  //Инициализирует библиотеку Wire и подключается к шине I2C
  Wire.setClock(400000L);        //задаем частоту передачи данных 400кГц
  lcd.init();                    //инициализация lcd1602
  lcd.backlight();               //вкл подстветка
  pinMode(UP, INPUT_PULLUP);     //Устанавливает режим работы заданного порта(pin) как входа
  pinMode(DOWN, INPUT_PULLUP);   //Устанавливает режим работы заданного порта(pin) как входа
  pinMode(START, INPUT_PULLUP);  //Устанавливает режим работы заданного порта(pin) как входа
  pinMode(MOT1_DIR, OUTPUT);     //Устанавливает режим работы заданного порта(pin) как выхода
  pinMode(MOT1_STP, OUTPUT);     //Устанавливает режим работы заданного порта(pin) как выхода
  pinMode(NAGREV, OUTPUT);       //Устанавливает режим работы заданного порта(pin) как выхода
  pinMode(GERKON, INPUT_PULLUP); //Устанавливает режим работы заданного порта(pin) как выхода
  x = 0;                         //обнуляем
  y = 0;                         //обнуляем
}

void loop()
{
  staticmenu(); //вызов функции Надпись дисплея "НАМОТКА"

  if (!digitalRead(UP) && x == 0) //если нажата кнопка UP
  {
    staticmenu();                 //вызов функции
    y += 1;                       //прибавляем один виток при нажатии кнопки UP
    lcd.setCursor(6, 1);          //ставим курсор на 6 позицию 1 строки
    lcd.print(y, DEC);            //на экран в десятичном виде выдать значение переменной y
  }

  if (!digitalRead(DOWN) && x == 0) //если нажата кнопка DOWN
  {
    staticmenu();                 //вызов функции
    y -= 1;                       //вычитаем один виток при нажатии кнопки DOWN
    lcd.setCursor(6, 1);          //ставим курсор на 6 позицию 1 строки
    lcd.print(y, DEC);            //вывести на экран и затереть цифры с права от y (остатки от предыдущего y)
    lcd.print(" ");
  }

  if (x == 0 && y >= 1) //если x=0 и введено некоторое количество витков y больше или равно 1
  {
    if (!digitalRead(START)) //если нажата кнопка START
    {
      do //исключаем многокраную сработку
      {
        /* code */                     //кнопка START нажата многократно? Да-вверх, Нет-вниз
      } while (!digitalRead(START)); 
      lcd.clear();                     //очищаем дисплей
      staticmenu();                    //выводим первоначальный дисплей
      digitalWrite(MOT1_STP, HIGH);    //включаем мотор, HIGH на выходе MOT1_STP будет 5В
      stepA();                       //вызвать функцию stepA
    }
  }
}

void staticmenu() // Надпись дисплея "НАМОТКА"
{
  lcd.setCursor(0, 0);
  lcd.println(" HAMOTKA CUPRUM "); //Надпись названия программы сверху
}

void stepA() //сама функция stepA
{
  x = 1;

  do //цикл идет до тех пор пока CountGerkon не сравняется с установленным количеством намотки y
  {


    //////обработка геркона (оптодатчика)
    if (digitalRead(GERKON)) // если сработал датчик геркон или оптодатчик
    {
      do //исключаем многократную сработку
      {
        /* code */                   //геркон сработал многократно? Да-вверх, Нет-вниз
      } while (digitalRead(GERKON)); 
      CountGerkon += 1;              //прибавляем один при срабатывании геркона/оптодатчика
      lcd.setCursor(6, 1);           //ставим курсор на 6 позицию 1 строки
      lcd.print(CountGerkon, DEC);   //на экран в десятичном виде выдать значение переменной y

      //качание каретки
      if (j) //организуем цикл для сервопривода качания каретки вправо влево
      {
        if (g >= l)                //крайняя левая точка в градусах 
        {
          j = 0;                   //качание вправо
          g = g - q;               //вычесть градус смещения (0,62) на 1 виток
        }
        else
          g = g + q;               //прибавить градус смещения (0,62) на 1 виток
      }
      else
      {
        if (g <= r)                //крайняя правая точка в градусах 
        {
          j = 1;                   //качание влево 
          g = g + q;               //прибавить градус смещения (0,62) на 1 виток
        }
        else
          g = g - q;               //вычесть градус смещения (0,62) на 1 виток
      }
      // END качание каретки
    }
    //////end обработки геркона (оптодатчика)

    lcd.setCursor(6, 1);          //курсор поставить на 6 позицию 1 строки
    lcd.print(CountGerkon, DEC);  //печать на экран фактического текущего значения витков переменной a
    servo.write(g);               // Поворачиваем серво на g градусов  
  } while (CountGerkon <= y - 1);
  digitalWrite(MOT1_STP, LOW); //Установка значения LOW приведет к тому, что напряжение на выходе MOT1_DIR будет 0В
  
}
