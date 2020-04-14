//============================================================================================================================
//
//                                        PROGRAMA RESPIRADOR SOL AR
//
//           https://github.com/tiagocriaar/AMBU-SOL-AR
//
//           Arduino Mega 2560  - Arduino IDE 1.8.10
//           Motor de Passo NEMA23 15 kgf.cm / Driver de motor WD2404
//           Display LCD 20x4 I2C / teclado com 16 Teclas -  Keypad 4x4
//
//============================================================================================================================
 long int versao_SOLAR     =  1;
 long int sub_versao_SOLAR = 36;

//   DATA: 13/04/20
//   HORA: 21:00
//
//
//   ANALISTA
//   DE SISTEMAS:   SÉRGIO GOULART ALVES PEREIRA
//
//   PROGRAMADORES: SÉRGIO GOULART ALVES PEREIRA
//                  FELIPE JONATHAN DIAS TOBIAS
//                  JOSÉ GUSTAVO ABREU MURTA
//
//   REVISORES:     JOSÉ GUSTAVO ABREU MURTA
//                  SÉRGIO GOULART ALVES PEREIRA
//
//   IMPLEMENTAÇÃO:
//
//   COLOCAR O TEMPO DE PLATÔ PARA UM CICLO 
//
//
//============================================================================================================================
//   obs: a Biblioteca SpeedyStepper não permite mudança de velocidade durante o percurso, por isso mudei
//
//   https://github.com/mathertel/LiquidCrystal_PCF8574   Biblioteca do Display
//   http://playground.arduino.cc/Code/Keypad             Biblioteca do teclado
//   https://github.com/Stan-Reifel/FlexyStepper          Biblioteca FlexyStepper
//
//============================================================================================================================
// Bibliotecas
//============================================================================================================================

   #include <Arduino.h>
   #include <TimerOne.h>
   #include <Wire.h>                        // Biblioteca Wire 
   #include <Keypad.h>                      // Biblioteca para teclado 
   #include <LiquidCrystal_PCF8574.h>       // Biblioteca para LCD com I2C

   #include <FlexyStepper.h>                // Biblioteca controle do motor de passo

//============================================================================================================================
// Rótulos das pinagens
//============================================================================================================================

  #define     potenciometro_MP A0          // POTENCIÔMETRO VEL MOTOR DE PASSO 
  #define     P_AMBU           A1          // Leitura de pressão do Ambu
  #define     P_paciente       A2          // Leitura de pressão do Paciente
  #define     P_vacuo          A3          // Leitura de vácuo
  #define     potenciometro_TP A4          // Regulagem de tempo do platô 

  // TX0                        0 // FUTURO
  // RX0                        1 // FUTURO
  //?? RS485                    2 // FUTURO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ESCOLHER OUTRA

  #define     CH_Avanca_MP      2          // Botão de avanço do motor de passo (pino 2)
  #define     CH_Recua_MP       3          // Botão de recuo do motor de passo (pino  3)
  #define     pulso_MP          5          // Trem de pulso pra ajuste de velocidade (pino 5)

  // RS485                     14 // FUTURO
  // RS485                     15 // FUTURO

  //deleta #define     Btn_Liga_MP      18          // Botão para ligar o motor de passo (pino 18) Alterei essa porta 
  //deleta #define     Btn_Para_MP      19          // Botão de parada do motor de passo (pino 19) Alterei essa porta 

  // DISPLAY SDA               20          // DISPLAY SDA
  // DISPLAY SCL               21          // DISPLAY SCL

  #define     enable_MP        24          // Habilita o driver do motor de passo (pino 24)
  #define     dir_MP           25          // Define a direçao do motor de passo  (pino 25)

  #define     Falha_E          28          // Sinal de falha de energia (ligar nobreak)
  #define     TESTE_G          29          // Botão de teste geral
  #define     R1_GER           30          // Relé Geral
  #define     R2_AVAN          31          // Relé de Avanço do motor de passo
  #define     R3_RET           32         // Relé de retorno do motor de passo
  #define     R4_V_AMBU        33          // Relé de Valvula do Ambu
  #define     R5_V_02          34          // Relé de oxigênio
  #define     R6_V_AR          35          // Relé de Ar 
  #define     R7_BYPASS        36          // Relé de By-pass

  // TECLADO  LIN1(D38)        38
  // TECLADO  LIN2(D40)        40
  // TECLADO  LIN3(D42)        42
  // TECLADO  LIN4(D44)        44
  // TECLADO  COL1(D46)        46
  // TECLADO  COL2(D48)        48
  // TECLADO  COL3(D50)        50
  // TECLADO  COL4(D52)        52

  #define     CFC_Inicio       39          // Chave fim de curso início (home position) 
  #define     CFC_Fim          41          // Chave fim de curso fim (limte pressão do AMBU) 

//============================================================================================================================
// Variáveis globais
//============================================================================================================================

 int   valor_pot_MP        = 1000;  // Vai receber a leitura do potenciometro do MP
 int   frequencia_MP       = 2000;  // Vai receber valor de frequencia para ajuste de velocida

 float valor_pot_TP  = analogRead(potenciometro_TP);   // Lê o valor da tensão no potenciometro do tempo de platô
 float Tempo_seg_Plato_NOK = map(valor_pot_TP, 0, 1023, 0, 1000);   // Converte a tensão em tempo em ms

 float Tempo_seg_Plato_OK  =  223;  // Tempo de platô  - convertido em segundos após o confirma

 float Tins            =    0;  // Tempo de inspiração  geralmente 1.0 s
 float Texp            =    0;  // Tempo de expiração   geralmente 0.5 s
 float TPlato          =    0;  // Tempo de platô  - estado entre inspiração e expiração - ver uma variação 0s ou ... ou .. 0.7 s
 
 int   cod_causa       =    0;
 char  tag_causa       =   "";
 char  nome_causa_LN2  =   "";
 char  nome_causa_LN3  =   "";
 int   cod_efeito      =    0;
 char  tag_efeito      =   "";
 char  nome_efeito_LN2 =   "";
 char  nome_efeito_LN3 =   "";

// Mudança do endereço do LCD
 LiquidCrystal_PCF8574 lcd(0x27);        // Endereço I2C - LCD PCF8574

 const byte linhas  = 4;                 // linhas do teclado
 const byte colunas = 4;                 // colunas do teclado
 char Keys[linhas][colunas] =            // Definicao dos valores dos botoes 1 a 6
 {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
 };

 byte LinhaPINO[linhas] =   {38, 40, 42, 44};      // Linhas do teclado D38 a D44
 byte ColunaPINO[colunas] = {46, 48, 50, 52};      // Colunas do teclado D46 a D52

 Keypad keypad = Keypad( makeKeymap(Keys), LinhaPINO, ColunaPINO, linhas, colunas );  // configuração do teclado

 //char key = keypad.getKey();            // faz varredura dos botões do teclado

 
 boolean blink = false;                  // variavel blink
 int       StepsPerRevolution    = 400;  // passos por volta do motor - modo 1/2 micropasso
 int       directionTowardHome   = -1;   // sentido do motor = 1 para Ambu, -1 para home
 float     speedInStepsPerSecond;        // velocidade em passos/segundo
 bool      limitSwitchFlag;              // flag para switch de limite
 FlexyStepper stepper;                   // cria objeto stepper
//============================================================================================================================
// Declaração das funções - VOIDS
//============================================================================================================================

 void LCD_Inicial (long int versao_SOLAR, int sub_versao_SOLAR);

 //-----------------------------------------------------------------------------------------------------------------------------------------
 // Declaração voids do motor de passo
 
    void LCD_procedimento_ligar_MP      ();        // Liga o motor
    void LCD_procedimento_desligar_MP   ();        // Para o fuso
    void LCD_procedimento_avanca_MP     ();        // Avançar o fuso
    void LCD_procedimento_recua_MP      ();        // Recua o fuso
    void procedimento_ler_pot_MP(float valor_pot_MP,float frequencia_MP); // Ler o potenciometro do motor de passo
    void LCD_Teste_MP               ();   
    void LCD_HomePosition           ();
    void LCD_iniciaMotor            ();
    void LCD_CFC_inicio             ();
    void LCD_CFC_fim                ();

    void LCD_pressionaAmbu          (); 
    void LCD_soltaAmbu              ();
    void LCD_Ligando_MP             ();
    void LCD_Desligando_MP          ();
 
 //-----------------------------------------------------------------------------------------------------------------------------------------
 // Declaração voids do tempo de platô

    void LCD_Teste_Potenciometro_TP ();        // Tela de teste do potenciometro do Tempo de Plato
    void procedimento_ler_pot_TP    ();
                                               // Lê a entrada do potenciômetro do Tempo de Plato
    void LCD_Mostra_Valor_Potenciometro_TP (); 
                                               // Mostra o valor do potenciômetro para o plato 
    void LCD_Tecla_A_Aceitar        ();        // Tela mensagem Tecla A para aceitar
    void LCD_TECLA_A_Pressionada    ();        // Confirma tecla A pressionada para aceitar o valor do Tempo de Plato
    void LCD_Pausa_Plato            ();        // mostra tela pausa de platô

 //-----------------------------------------------------------------------------------------------------------------------------------------
 // Declaração voids do teclado
    void keypadEvent(KeypadEvent key);

 //-----------------------------------------------------------------------------------------------------------------------------------------
 // Declaração voids causas e efeitos
 
    void procedimento_tag_causa(int  cod_causa,  char tag_causa,  char nome_causa_LN2,  char nome_causa_LN3);  // Lê a tipo de causa
    void procedimento_tag_efeito(int cod_efeito, char tag_efeito, char nome_efeito_LN2, char nome_efeito_LN3); // Lê o tipo de defeito

    void LCD_Mostra_causa(int cod_causa, char tag_causa, char nome_causa_LN2, char nome_causa_LN3);
 

 //-----------------------------------------------------------------------------------------------------------------------------------------
 // Declaração voids Demos

    void Demo_Lista_Causas(int cod_causa, char tag_causa, char nome_causa_LN2, char nome_causa_LN3);// Demo Lista de Causas
    void Demo_Telas_MP (long int versao_SOLAR, long int sub_versao_SOLAR);                          // Demo das telas de testes MP

 
//============================================================================================================================
 void setup()
//============================================================================================================================
 {
  Serial.begin(9600);                    // Habilita a comunicação com a velocidade de 9600 bits por segundo
  pinMode (potenciometro_MP, INPUT);     // Define o pino A0 como entrada analógica do potenciometro
  pinMode (CH_Avanca_MP,    INPUT);     // Define o pino 21 como entrada digital do botão Avança
  pinMode (CH_Recua_MP,     INPUT);     // Define o pino 20 como entrada digital do botão Recua
 
  //deleta  pinMode (Btn_Para_MP,      INPUT);     // Define o pino 19 como entrada digital do botão Parar
  //deleta  pinMode (Btn_Liga_MP,      INPUT);     // Define o pino 18 como entrada digital do botão Recua
 
  pinMode (enable_MP,       OUTPUT);     // Define o pino 24 como saída digital de habilitação do driver
  pinMode (dir_MP,          OUTPUT);     // Define o pino 25 como saída digital de sentido de giro
  pinMode (pulso_MP,        OUTPUT);     // Define 3 pino como saída do trem de pulso

  digitalWrite  (CH_Avanca_MP,INPUT_PULLUP);      // Ativa pull-up do pino 2 (Avança)
  digitalWrite  (CH_Recua_MP, INPUT_PULLUP);      // Ativa pull-up do pino 3 (Recua)
  //deleta  digitalWrite  (Btn_Liga_MP,  INPUT_PULLUP); // Ativa pull-up do pino 18 (Ligar)
  //deleta  digitalWrite  (Btn_Para_MP,  INPUT_PULLUP); // Ativa pull-up do pino 19 (Parar)

  digitalWrite  (enable_MP, HIGH);        // Desabilita driver
  digitalWrite  (dir_MP, HIGH);       // Define sentido de giro inicial como avançar

  stepper.setStepsPerRevolution (StepsPerRevolution);   // configura passos por volta para o Driver do motor
  stepper.connectToPins(pulso_MP , dir_MP);             // configura pinos no driver WD2404

  int error;                             // variável de erro para o Display LCD

  Wire.begin();                          // Inicializa interface do LCD
  Wire.beginTransmission(0x3F);          // incializa interface I2C
  error = Wire.endTransmission();        // termina transmissão se houver erro

  lcd.begin(20, 4);                      // inicializa Display LCD 20x4
  LCD_Inicial (versao_SOLAR, sub_versao_SOLAR);            // Tela inicial no display LCD
  delay (2000);                          // atraso de 2 segundos

  //----------------------------------------------------------------------------------------------------------------------------------------
  //Lógica para regular o potenciometro do Tempo de Platô
 
  valor_pot_TP  = analogRead(potenciometro_TP);   // Lê o valor da tensão no potenciometro do tempo de platô
  Tempo_seg_Plato_NOK = map(valor_pot_TP, 0, 1023, 0, 1000);   // Converte a tensão em tempo em ms
  
  LCD_Teste_Potenciometro_TP ();
  delay (5000);                          // atraso de 5 segundos

  //Fazendo a leitura do potenciometro do platô
 
  // Mensagem do tempo de platô lido no potenciômetro
  lcd.setBacklight(255);                     // luz de fundo LCD
  lcd.home(); lcd.clear();                   // limpa tela LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("      O tempo       ");                      // mostra no LCD  
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print(" de plato e: "+String(Tempo_seg_Plato_NOK)+" ms");                      // mostra no LCD  

                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("     Tecle em A     ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("  Para aceitar, OK  ");                      // mostra no LCD
 
  delay (2000);                              // atraso de 2 segundos
 
  lcd.home(); lcd.clear();                   // limpa tela LCD

  procedimento_ler_pot_TP;

  delay (2000);                              // atraso de 2 segundos
  LCD_Tecla_A_Aceitar();

  delay (2000);                              // atraso de 2 segundos
  LCD_TECLA_A_Pressionada();
  
  LCD_Mostra_Valor_Potenciometro_TP (); // Mostra o valor do potenciômetro para o plato 
  delay (2000);                              // atraso de 2 segundos

  //----------------------------------------------------------------------------------------------------------------------------------------
  char key = keypad.getKey();            // faz varredura dos botões do teclado
 
  //teste 31
  keypadEvent(key);

  LCD_Teste_MP ();                    // Tela de testes do MP
  delay (1000);                         // atraso de 2 segundos
 
  keypad.addEventListener(keypadEvent);  // evento para leitura do teclado

//??  LCD_Mostra_Lista_Causas (0,0,0,0);
  
  //------------------------------------------------------------------------------------------------------------------------------------------
  // Prototipo de interrupções

  // Observação - os pinos D22, D23, D24 e D25 não suportam interrupções usando Arduino IDE
  // Para Arduino MEGA somente esses pinos podem ser usados  - 2, 3, 18, 19, 20, 21
  // https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

  attachInterrupt(digitalPinToInterrupt(CH_Avanca_MP), LCD_procedimento_avanca_MP,FALLING);   // Habilita interrupção no botão de avanço
  attachInterrupt(digitalPinToInterrupt(CH_Recua_MP),  LCD_procedimento_recua_MP, FALLING);   // Habilita interrupção no botão de recuo

  Timer1.attachInterrupt(procedimento_ler_pot_MP);                                                // Habilita interrupção no Timer1
  Timer1.initialize(200000);                                                                   // Carrega tempo no timer (em microssegundos)

 } //FIM do void setup

//============================================================================================================================
 void loop()
//============================================================================================================================
 {
//ver  lcd.begin(20, 4);                      // inicializa Display LCD 20x4
//ver  LCD_Inicial (versao_SOLAR, sub_versao_SOLAR);            // Tela inicial no display LCD
//ver  delay (2000);                          // atraso de 2 segundos

//ver  LCD_Teste_Potenciometro_TP ();
//ver  delay (5000);                         // atraso de 5 segundos

//ver  LCD_Mostra_Valor_Potenciometro_TP (); // Mostra o valor do potenciômetro para o plato 
 //ver delay(1000);

  char key = keypad.getKey();                // faz varredura dos botões do teclado
 
  //teste 32
  keypadEvent(key);

 keypad.addEventListener(keypadEvent);  // evento para leitura do teclado

 //ver  delay (1000);                        // atraso de 1 segundo

//ver  LCD_Teste_MP ();                      // Tela de testes do MP
//ver  delay (5000);                         // atraso de 5 segundos

  tone(pulso_MP, frequencia_MP);             // Envia trem de pulso para o driver]

  
 } // FIM do void loop

//------------------------------------------------------------------------------------------------------------------------------------------







//OK ------------------------------------------------------------------------------------------------------------------------------------------
// Descrição das funções
 void LCD_procedimento_avanca_MP()
 {
  digitalWrite(dir_MP   , HIGH);            // Define sentido de giro para avançar fuso
 
  lcd.setBacklight(255);              // luz de fundo LCD
  lcd.home(); lcd.clear();            // limpa tela LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("        Fuso        ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 3
  lcd.print("     avançando      ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                      // mostra no LCD
 
  delay (2000);                              // atraso de 2 segundos
 }
//OK ------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_procedimento_recua_MP()
 {
  digitalWrite(dir_MP   , LOW);            // Define sentido de giro para avançar fuso
 
  lcd.setBacklight(255);              // luz de fundo LCD
  lcd.home(); lcd.clear();            // limpa tela LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("        Fuso        ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 3
  lcd.print("      recuando      ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                      // mostra no LCD
 
  delay (2000);                              // atraso de 2 segundos
  }
//OK ------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_procedimento_ligar_MP()
 {
  digitalWrite(enable_MP, HIGH);         // Habilita driver

  lcd.setBacklight(255);              // luz de fundo LCD
  lcd.home(); lcd.clear();            // limpa tela LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("       Motor        ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 3
  lcd.print("       Ligado       ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                      // mostra no LCD
 
  delay (2000);                              // atraso de 2 segundos
 }
//OK ------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_procedimento_desligar_MP()
 {
  digitalWrite(enable_MP, LOW);          // Desabilita driver
 
  lcd.setBacklight(255);              // luz de fundo LCD
  lcd.home(); lcd.clear();            // limpa tela LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("       Motor        ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 3
  lcd.print("     desligado      ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                      // mostra no LCD
 
  delay (2000);                              // atraso de 2 segundos
 } 
  
//OK ------------------------------------------------------------------------------------------------------------------------------------------
 void procedimento_ler_pot_MP(float valor_pot_MP,float frequencia_MP)
 {
  valor_pot_MP  = analogRead(potenciometro_MP);   // Lê o valor da tensão no potenciometro
  frequencia_MP = map(valor_pot_MP, 0, 1023, 2000, 4000);   // Converte a tensão em frequencia

  //  lixo
  //  frequencia_MP = ((valor_pot_MP + 1000) * 2);   // Converte a tensão em frequencia
  //  if (valor_pot_MP > 1000)              // Limita a leitura de tensão no potenciometro em 1000
  //  {
  //    valor_pot_MP = 1000;
  //  }
 }
//OK ------------------------------------------------------------------------------------------------------------------------------------------
// Controle do Motor de Passo
 void disableWD2404()
 {
  digitalWrite(enable_MP, LOW);          // Desativa o driver WD2404 (verifique se o nivel logico esta correto)
  delay (10);                            // Atraso de 10 milisegundos
 }
//OK -------------------------------------------------------------------------------------------------------------------------------------------
 void enableWD2404()
 {
  digitalWrite(enable_MP, HIGH);         // Ativa o driver WD2404   (verifique se o nivel logico esta correto)
  delay (10);                            // Atraso de 10  milisegundos
 }
//OK -------------------------------------------------------------------------------------------------------------------------------------------
//  Comprimento do eixo sem fim = 200 mm
//  Curso para compressão do AMBU = 100 mm
//  Driver WED2404 configurado para 1/2 passo = 400 passos/volta
//  Velocidade 400 passos/segundo corresponde à uma Volta/segundo (altere essa velocidade depois dos testes)
//  Para avançar os 100 mm são necessários 20 voltas no motor ou 20 x 400 passos = 8000 passos
//  Faça testes iniciais do motor sem o mecanismo!

//OK -------------------------------------------------------------------------------------------------------------------------------------------
 void motorCFCinicio()                    // posiciona a aba na posição inicial
 {
  directionTowardHome = -1;                     // sentido do motor = -1 para posição inicial
  enableWD2404();                                                  // ativa driver do motor
  stepper.setAccelerationInStepsPerSecondPerSecond(400);           // configura aceleraçao do motor          (tem que ajustar esses valores)
  long maxDistanceToMoveInSteps = 8000;  // maxima distancia percorrida em passos  (100 mm = 8000 passos - Atenção no teste)

  if (digitalRead(CFC_Inicio) == HIGH)   // se chave CFC_Inicio não foi acionada
  {
    stepper.setSpeedInStepsPerSecond(400); // configura velocidade do motor em passos/segundo (tem que ajustar esses valores)
    stepper.setTargetPositionRelativeInSteps(maxDistanceToMoveInSteps * directionTowardHome);     // movimenta motor no sentido para o início

    limitSwitchFlag = false;
    while (!stepper.processMovement())   // enquanto estiver girando o motor
    {
      if (digitalRead(CFC_Inicio ) == LOW)                         // se a chave CFC_Inicio for acionada
      {
        limitSwitchFlag = true;          // muda o estado do flag
        lcd.setCursor(2, 3);             // LCD coluna 2 e linha 3
        lcd.print("CFC Inicio OK    ");  // mostra no LCD
        break;                           // para de girar motor
      }
    }
    if (limitSwitchFlag == false)        // se a chave CFC_Inicio nunca for acionada
    {
      lcd.setCursor(2, 3);               // LCD coluna 0 e linha 0
      lcd.print("CFC Inicio not OK");    // mostra no LCD
    }
  }
  stepper.setCurrentPositionInSteps(0L); // encontrou home - zera a posição em passos
  delay(500);                            // atraso 500 ms
}
//-----------------------------------------------------------------------------------------------------------------------------------------
void motorPressionaAmbu ()               // movimenta o motor no sentido do Ambu - Faça testes iniciais do motor sem o mecanismo!
{
  bool stopFlag = false;
  enableWD2404();                        // ativa driver do motor

  directionTowardHome = 1;               // sentido do motor = 1 para Ambu
  stepper.setCurrentPositionInSteps(0);  // zera o contador de passos - para o motor
  stepper.setTargetPositionInSteps(8000);// objetivo do percurso 100 mm (8000 passos) - Atenção no teste
  stepper.setSpeedInStepsPerSecond(400); // velocidade do motor em passos/segundo  (tem que ajustar esses valores)
  stepper.setAccelerationInStepsPerSecondPerSecond(600);                 // configura aceleraçao do motor          (tem que ajustar esses valores)

  while (!stepper.motionComplete())      // enquanto o motor não avançar todo percurso
  {
    stepper.processMovement();           // gira o motor

    long currentPosition = stepper.getCurrentPositionInSteps();          // verifica a posição percorrida em passos
    if (currentPosition == 400)                                          // se a posição precorrida for igual a 400 (tem que ajustar esses valores)
      stepper.setSpeedInStepsPerSecond(800);                             // aumente a velocidade (tem que ajustar esses valores)

    if ((digitalRead(CFC_Fim) == LOW) && (stopFlag == false))            // se a chave CFC_Fim for acionada
    {
      stepper.setTargetPositionToStop();
      stopFlag = true;                   // altera o estado do flag
    }
  }
  delay (2000) ;                         // atraso de 2 segundos
 }


//---------------------------Telas do Display LCD -------------------- Sugestões de Telas para Display
 void LCD_Inicial (long int versao_SOLAR, long int sub_versao_SOLAR)    // Tela inicial no display LCD
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                      lcd.setCursor( 0, 0); // coluna 0 e linha 0
  lcd.print("*    Ventilador    *");                      // mostra no LCD
                                      lcd.setCursor( 0, 1); // coluna 1 e linha 1
  lcd.print("*   Ambu Sol e AR  *");                      // mostra no LCD
                                      lcd.setCursor( 0, 2); // coluna 1 e linha 2
  lcd.print("*   Versao: ");                      // mostra no LCD
                                      lcd.setCursor(12, 2); // coluna 1 e linha 2
  lcd.print(versao_SOLAR);
                                      lcd.setCursor(13, 2); // coluna 1 e linha 2
  lcd.print(".");                      // mostra no LCD
                                      lcd.setCursor(14, 2); // coluna 1 e linha 2
  lcd.print(sub_versao_SOLAR);
                                      lcd.setCursor(19, 2); // coluna 1 e linha 2
  lcd.print("*");                      // mostra no LCD
                                      lcd.setCursor( 0, 3); // coluna 0 e linha 3

                                      
  lcd.print("*   Abril - 2020   *");                      // mostra no LCD
  delay (500);                              // atraso 0,5 segundos
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Teste_MP ()                    // Tela de testes do MP
 {
  lcd.setBacklight(255);                 // luz de fundo LCD
  lcd.home(); lcd.clear();               // limpa tela LCD

  LCD_iniciaMotor();
 
  delay (500);                              // atraso 0,5 segundos
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Teste_Potenciometro_TP ()    // Tela de teste do potenciometro do Tempo de Plato
 {
  lcd.setBacklight(255);              // luz de fundo LCD
  lcd.home(); lcd.clear();            // limpa tela LCD
  
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("      Ajuste o      ");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("   potenciometro    ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 1
  lcd.print("                    ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 2
  lcd.print(" do tempo de plato  ");                      // mostra no LCD

  delay (500);                              // atraso 0,5 segundos
 } 

//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Tecla_A_Aceitar ()        // Tela mensagem Tecla A para aceitar
 {                                     
  lcd.setBacklight(255);              // luz de fundo LCD
  lcd.home(); lcd.clear();            // limpa tela LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("      Tecle A       ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 3
  lcd.print("    para aceitar    ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                      // mostra no LCD
 
  delay (500);                              // atraso 0,5 segundos
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void procedimento_ler_pot_TP()
 {
   valor_pot_TP  = analogRead(potenciometro_TP);   // Lê o valor da tensão no potenciometro do tempo de platô
  
   Tempo_seg_Plato_NOK = map(valor_pot_TP, 0, 1023, 0, 10000);   // Converte a tensão em frequencia
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Mostra_Valor_Potenciometro_TP () // Mostra o valor do potenciômetro para o plato 
 {
  lcd.setBacklight(255);                     // luz de fundo LCD
  lcd.home(); lcd.clear();                   // limpa tela LCD
  
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
   lcd.print("  O tempo de plato  ");                      // mostra no LCD  
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
   lcd.print("do botao e: "+String(Tempo_seg_Plato_NOK)+" ms");                      // mostra no LCD  

                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("     Tecle em A     ");                      // mostra no LCD

                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("    Para aceitar    ");                      // mostra no LCD
 
  delay (500);                              // atraso 0,5 segundos
  }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_TECLA_A_Pressionada ()
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
  Tempo_seg_Plato_OK=Tempo_seg_Plato_NOK;
  
  lcd.setBacklight(255);                     // luz de fundo LCD
  lcd.home(); lcd.clear();        ;           // limpa tela LCD
                                        lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("  O tempo de plato  ");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("  configurado foi:  ");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("                    ");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("        "+String(Tempo_seg_Plato_OK)+" ms ");                  // mostra no LCD
  delay (500);                              // atraso 0,5 segundos
  }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_HomePosition ()                     // Print Home Position no LCD
 {
  lcd.setBacklight(255);                     // luz de fundo LCD
  lcd.home(); lcd.clear();                   // limpa tela LCD

                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                      // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*  Ambu  Sol e AR  *");                      // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*  Home  Position  *");                      // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                      // mostra no LCD
 
  delay (500);                              // atraso 0,5 segundos
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_iniciaMotor ()                      // Print Inicia Motor
 {
  lcd.setBacklight(255);                     // luz de fundo LCD
  lcd.home(); lcd.clear();                   // limpa tela LCD
                              lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("  Escolha a opcao:  ");                       // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("  [1] para iniciar  ");                       // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("  [2] para parar    ");                        // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("  [3] ciclo continuo");                        // mostra no LCD
  
  delay (500);                               // atraso 0,5 segundos
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_CFC_inicio ()                       // mostra CFC_Início
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD

                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*                  *");                   // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("* CFC_Inicio = ON  *");             // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                   // mostra no LCD
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_CFC_fim ()                           // mostra CFC_Fim
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*                  *");                   // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*   CFC_Fim = ON   *");             // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                   // mostra no LCD
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_pressionaAmbu ()                    // mostra Pressiona Ambu
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*    Pressiona     *");           // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*     o Ambu       *");           // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                   // mostra no LCD
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_soltaAmbu ()                        // mostra Solta Ambu
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                       lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD
                                       lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*      Solta       *");           // mostra no LCD
                                       lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*     o Ambu       *");           // mostra no LCD
                                       lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print("--------------------");                   // mostra no LCD
 }

//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Ligando_MP()                           // mostra tela ligando motor de passo
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
  
  lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD
                                       lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*     Ligando o    *");           // mostra no LCD
                                       lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*  motor de Passo  *");           // mostra no LCD
                                       lcd.setCursor(0, 3);                       // coluna 0 e linha 3
 lcd.print("--------------------");                   // mostra no LCD
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Desligando_MP()                           // mostra tela desligando motor de passo
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                      lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD
                                      lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*   Desligando o   *");           // mostra no LCD
                                      lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*  motor de Passo  *");           // mostra no LCD
                                      lcd.setCursor(0, 3);                       // coluna 0 e linha 3
 lcd.print("--------------------");                   // mostra no LCD
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Pausa_Plato()                           // mostra tela pausa de platô
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
                                     lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("--------------------");                   // mostra no LCD
                                     lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("*      Tempo       *");           // mostra no LCD
                                     lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print("*    de plato      *");           // mostra no LCD
                                     lcd.setCursor(0, 3);                       // coluna 0 e linha 3
 lcd.print("--------------------");                   // mostra no LCD
 }


//------------------------------------------------------------------------------------------------------------------------------------------
 void LCD_Mostra_Causa(int cod_causa, char tag_causa, char nome_causa_LN2, char nome_causa_LN3)
 {
  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD
 
                                     lcd.setCursor(0, 0);                       // coluna 0 e linha 0
  lcd.print("cod= "+(cod_causa));                   // mostra no LCD
                                     lcd.setCursor(0, 1);                       // coluna 0 e linha 1
  lcd.print("tag= "+(tag_causa));           // mostra no LCD
                                     lcd.setCursor(0, 2);                       // coluna 0 e linha 2
  lcd.print(""+(nome_causa_LN2));           // mostra no LCD
                                     lcd.setCursor(0, 3);                       // coluna 0 e linha 3
  lcd.print(""+(nome_causa_LN3));                   // mostra no LCD
 
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void vazio()
 {


  
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void vazio2()
 {


  
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void vazio3()
 {


  
 }
//------------------------------------------------------------------------------------------------------------------------------------------



//------------------------------------------------------------------------------------------------------------------------------------------
 void procedimento_tag_causa(int cod_causa, char tag_causa, char nome_causa_LN2, char nome_causa_LN3) // Lê o tipo de causa
 {
  // Mostra o Tag da causa
  //limite de 20 caracteres                             12345678901234567890                   12345678901234567890
  if (cod_causa = 0) {
    tag_causa = "SAL"  ;
    nome_causa_LN2 = "    Sem alarmes     ";
    nome_causa_LN3 = "                    ";
  }
  if (cod_causa = 1) {
    tag_causa = "SITN" ;
    nome_causa_LN2 = "  Situação normal   ";
    nome_causa_LN3 = "                    ";
  }
  if (cod_causa = 2) {
    tag_causa = "FET"  ;
    nome_causa_LN2 = "Falta energia tomada";
    nome_causa_LN3 = "                    ";
  }
  if (cod_causa = 3) {
    tag_causa = "AEQ"  ;
    nome_causa_LN2 = "Apnéia - equipamento";
    nome_causa_LN3 = "                    ";
  }
  if (cod_causa = 4) {
    tag_causa = "DCR"  ;
    nome_causa_LN2 = " Desconexão circuito";
    nome_causa_LN3 = "   respiratório     ";
  }
  if (cod_causa = 5) {
    tag_causa = "OCR"  ;
    nome_causa_LN2 = " Obstrução circuito ";
    nome_causa_LN3 = "   respiratório     ";
  }
  if (cod_causa = 6) {
    tag_causa = "DPEEP";
    nome_causa_LN2 = "  Válvula do PEEP   ";
    nome_causa_LN3 = "    com defeito     ";
  }
  if (cod_causa = 7) {
    tag_causa = "PNEG" ;
    nome_causa_LN2 = " Pressão_negativa   ";
    nome_causa_LN3 = "Paciente não sedado ";
  }
  if (cod_causa = 8) {
    tag_causa = "VCAB" ;
    nome_causa_LN2 = " Volume corrente    ";
    nome_causa_LN3 = " abaixo do ajustado ";
  }
  if (cod_causa = 9) {
    tag_causa = "VMAB" ;
    nome_causa_LN2 = "  Volume minuto     ";
    nome_causa_LN3 = " abaixo do ajustado ";
  }
  if (cod_causa = 10) {
    tag_causa = "VMACL";
    nome_causa_LN2 = "  Volume minuto     ";
    nome_causa_LN3 = "  acima do limite   ";
  }
  if (cod_causa = 11) {
    tag_causa = "VCAL" ;
    nome_causa_LN2 = " Volume corrente    ";
    nome_causa_LN3 = "  acima do limite   ";
  }
  if (cod_causa = 12) {
    tag_causa = "MOTL" ;
    nome_causa_LN2 = "   Motor_lento      ";
    nome_causa_LN3 = "                    ";
  }
  if (cod_causa = 13) {
    tag_causa = "MOTD" ;
    nome_causa_LN2 = " Motor_disparado    ";
    nome_causa_LN3 = "                    ";
  }
  if (cod_causa = 14) {
    tag_causa = "PO2B" ;
    nome_causa_LN2 = "  Baixa pressão     ";
    nome_causa_LN3 = " de entrada de 02   ";
  }
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 void procedimento_tag_efeito(int cod_efeito, char tag_efeito, char nome_efeito_LN2, char nome_efeito_LN3)  // Lê o tipo de defeito
 {
  // Mostra o Tag do efeito
  //limite de 20 caracteres                                    12345678901234567890                    12345678901234567890

  if (cod_efeito =  0) {
    tag_efeito = "SEM_EF"  ;
    nome_efeito_LN2 = "    Sem efeitos     ";
    nome_efeito_LN3 = "                    ";
  }
  if (cod_efeito =  1) {
    tag_efeito = "AL_VD"   ;
    nome_efeito_LN2 = "    Alarme Verde    ";
    nome_efeito_LN3 = "                    ";
  }
  if (cod_efeito =  2) {
    tag_efeito = "AL_VM "  ;
    nome_efeito_LN2 = "  Alarme vermelho   ";
    nome_efeito_LN3 = "                    ";
  }
  if (cod_efeito =  3) {
    tag_efeito = "AL_AM"   ;
    nome_efeito_LN2 = "  Alarme amarelo    ";
    nome_efeito_LN3 = "                    ";
  }
  if (cod_efeito =  4) {
    tag_efeito = "LG_NBK"  ;
    nome_efeito_LN2 = "   Liga Nobreak     ";
    nome_efeito_LN3 = "                    ";
  }
  if (cod_efeito =  5) {
    tag_efeito = "AL_LNBK" ;
    nome_efeito_LN2 = "Alarme liga nobreak ";
    nome_efeito_LN3 = "                    ";
  }
  if (cod_efeito =  6) {
    tag_efeito = "AL_MQB"  ;
    nome_efeito_LN2 = "      Alarme:       ";
    nome_efeito_LN3 = "   motor quebrado   ";
  }
  if (cod_efeito =  7) {
    tag_efeito = "AL_OCA"  ;
    nome_efeito_LN2 = " Alarme obstrução na";
    nome_efeito_LN3 = " compressão do ambu ";
  }
  if (cod_efeito =  8) {
    tag_efeito = "AL_VZAM" ;
    nome_efeito_LN2 = " Alarme vazamento   ";
    nome_efeito_LN3 = "     no Ambu        ";
  }
  if (cod_efeito =  9) {
    tag_efeito = "AL_D_PB" ;
    nome_efeito_LN2 = " Alarme desconexão  ";
    nome_efeito_LN3 = "   (pressão baixa)  ";
  }
  if (cod_efeito = 10) {
    tag_efeito = "AL_PB"   ;
    nome_efeito_LN2 = "      Alarme:       ";
    nome_efeito_LN3 = " pressão baixa      ";
  }
  if (cod_efeito = 11) {
    tag_efeito = "AL_PP<PL";
    nome_efeito_LN2 = " Alarme pressão de  ";
    nome_efeito_LN3 = "   Ppico < Plimite  ";
  }
  if (cod_efeito = 12) {
    tag_efeito = "AL_PP>PL";
    nome_efeito_LN2 = " Alarme pressão de  ";
    nome_efeito_LN3 = "   Ppico > Plimite  ";
  }
  if (cod_efeito = 13) {
    tag_efeito = "AC_VMP"  ;
    nome_efeito_LN2 = "  Acionar válvula   ";
    nome_efeito_LN3 = " mecânica de pressão";
  }
  if (cod_efeito = 14) {
    tag_efeito = "RED_P<10";
    nome_efeito_LN2 = "  Reduzir pressão   ";
    nome_efeito_LN3 = " P menos de 10 cmH20";
  }
  if (cod_efeito = 15) {
    tag_efeito = "AL_PEEP" ;
    nome_efeito_LN2 = "    Alarme PEEP     ";
    nome_efeito_LN3 = "  acima do ajustado ";
  }
  if (cod_efeito = 16) {
    tag_efeito = "AL_PNEG" ;
    nome_efeito_LN2 = "      Alarme:       ";
    nome_efeito_LN3 = "  pressão negativa  ";
  }
  if (cod_efeito = 17) {
    tag_efeito = "ST_MAE"  ;
    nome_efeito_LN2 = "Status de movimento ";
    nome_efeito_LN3 = "   ambu => esvaziar ";
  }
  if (cod_efeito = 18) {
    tag_efeito = "AL_VMABA";
    nome_efeito_LN2 = "Alarme Volume mínimo";
    nome_efeito_LN3 = "  abaixo do ajustado";
  }
  if (cod_efeito = 19) {
    tag_efeito = "CVF"     ;
    nome_efeito_LN2 = "  Complementar o    ";
    nome_efeito_LN3 = "  volume fornecido  ";
  }
  if (cod_efeito = 20) {
    tag_efeito = "AL_VMACL";
    nome_efeito_LN2 = " Alarme Volume mim  ";
    nome_efeito_LN3 = "  acima do limite   ";
  }
  if (cod_efeito = 21) {
    tag_efeito = "AL_VCACL";
    nome_efeito_LN2 = "      Alarme:       ";
    nome_efeito_LN3 = "Vcorrente >  Vlimite";
  }
  if (cod_efeito = 22) {
    tag_efeito = "AL_C02VZ";
    nome_efeito_LN2 = "AL cilindro O2 vazio";
    nome_efeito_LN3 = "   falência de gás  ";
  }
  if (cod_efeito = 23) {
    tag_efeito = "AL_FR_AUM";
    nome_efeito_LN2 = "Alarme de frequência";
    nome_efeito_LN3 = "respiratória aument.";
  }
  if (cod_efeito = 24) {
    tag_efeito = "RED_VMOT";
    nome_efeito_LN2 = "Reduzir a velocidade";
    nome_efeito_LN3 = "     do motor       ";
  }
  if (cod_efeito = 25) {
    tag_efeito = "AUM_VMOT";
    nome_efeito_LN2 = "Aumentar  velocidade";
    nome_efeito_LN3 = "     do motor       ";
  }
 }
//------------------------------------------------------------------------------------------------------------------------------------------

// ------------------------- Funções do Teclado ----------------------
 void keypadEvent(KeypadEvent key)
 {
  switch (keypad.getState())                  // verifica se algum botão foi pressionado
  {
    case PRESSED:                             // identifica o botão pressionado

      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '1')                         // Botão 1 - LIGA
       {
        delay(1000);
        LCD_Ligando_MP(); 
 
        delay(1000);
        LCD_procedimento_ligar_MP ();
 
        delay (1000);
        LCD_procedimento_avanca_MP;
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '2')                         // Botao 2 - PARAR
       {
        delay(1000);
        LCD_Desligando_MP(); 
 
        delay(1000);
        LCD_procedimento_desligar_MP ();

        delay (1000);
        LCD_procedimento_recua_MP;            
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '3')                         // Botao 3 
       {
       //Movimentando um ciclo contínuo com Tempo de Plato
        LCD_Pausa_Plato();
        
        delay (Tempo_seg_Plato_NOK*10);
        LCD_Ligando_MP(); 

        delay(1000);
        LCD_procedimento_ligar_MP ();
  
        delay (1000);
        LCD_procedimento_avanca_MP;

        delay (Tempo_seg_Plato_NOK*10);

        LCD_Desligando_MP(); 
        delay(1000);
        LCD_procedimento_desligar_MP ();
 
        delay (1000);
        LCD_procedimento_recua_MP;            
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '4')                         // Botao 4
       {
        }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '5')                         // Botao 5
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '6')                         // Botao 6
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '7')                         // Botao 7
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '8')                         // Botao 8
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '9')                         // Botao 9
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '0')                         // Botao 0
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '*')                         // Botao *
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == '#')                         // Botao #
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == 'A')                         // Botao A - ACEITA O TEMPO DE PLATÔ
       {
        LCD_TECLA_A_Pressionada ();
        
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == 'B')                         // Botao B
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == 'C')                         // Botao C
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
      if (key == 'D')                         // Botao D
       {
        // insira funções aqui
       }
      //------------------------------------------------------------------------------------------------------------------------------------
  }
 } 
 //------------------------------------------------------------------------------------------------------------------------------------------
 void Demo_Telas_MP (long int versao_SOLAR, long int sub_versao_SOLAR)                          // Demo das telas de testes
 {
  lcd.begin(20, 4);                           // inicializa Display LCD 20x4

  lcd.setBacklight(255);                                  // apaga backlight
  lcd.home(); lcd.clear();                                // limpa display LCD

  LCD_Inicial (versao_SOLAR, sub_versao_SOLAR);        // Tela inicial no display LCD
  delay (3000);                               // atraso de 3 segundos
  LCD_HomePosition (); 
  delay (3000);                               // atraso de 3 segundos
  LCD_iniciaMotor ();
  delay (3000);                               // atraso de 3 segundos
  LCD_CFC_inicio (); 
  delay (3000);                               // atraso de 3 segundos
  LCD_CFC_fim ();
  delay (3000);                               // atraso de 3 segundos
  LCD_pressionaAmbu ();
  delay (3000);                               // atraso de 3 segundos
  LCD_soltaAmbu ();      
  delay (3000);                               // atraso de 3 segundos
  
  LCD_Teste_MP ();                            // Tela de testes do MP
  delay (3000);                               // atraso de 3 segundos
 }
 //------------------------------------------------------------------------------------------------------------------------------------------
 void Demo_Lista_Causas(int cod_causa, char tag_causa, char nome_causa_LN2, char nome_causa_LN3) // Demo Lista de Causas
 {
  cod_causa     = 0;
  tag_causa     = "";
  nome_causa_LN2 = "";
  nome_causa_LN3 = "";

 for (cod_causa==0; cod_causa<15;cod_causa++)
  {
   procedimento_tag_causa;
   LCD_Mostra_causa(cod_causa, tag_causa, nome_causa_LN2, nome_causa_LN3);
   delay(2000);
  }
 }
//------------------------------------------------------------------------------------------------------------------------------------------
 
 
 
