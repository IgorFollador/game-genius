//INCLUSÃO DE BIBLIOTECAS
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//DEFINIÇÕES
//LCD
#define endereco  0x3F  //Localização do lcd no módulo i2c
#define colunas   16    
#define linhas    2 
//LEDS
#define LED_VERDE 2
#define LED_AMARELO 3
#define LED_VERMELHO 4
#define LED_AZUL 5
//BOTÕES
#define BOTAO_VERDE 8
#define BOTAO_AMARELO 9
#define BOTAO_VERMELHO 10
#define BOTAO_AZUL 11
//MEMORIA RETENTIVA PARA RECORDE EEPROM
#define adress_SCORE 0x00 //Endereço onde será armazenada a informaçao dos pontos na memoria EEPROM
#define adress_configSound 0x04 //Endereço onde será armazenada as informações de configuração de som na memoria EEPROM
#define som 0x06

#define TAMANHO_SEQUENCIA 50 //DEFINE O NUMERO DE RODADAS

#define INDEFINIDO -1

LiquidCrystal_I2C lcd(endereco, colunas, linhas); //Realiza a configuração do lcd

enum Estados{             //Definimos os estados bases para funcionamento do jogo
    ABERTURA_DO_JOGO,
    PRONTO_PARA_PROX_RODADA,
    USUARIO_RESPONDENDO,
    JOGO_FINALIZADO_SUCESSO,
    JOGO_FINALIZADO_FALHA
};

int sequenciaLuzes[TAMANHO_SEQUENCIA];
int rodada = 0;
int leds_respondidos = 0;
int pontos = 0;
int recorde = 0;
bool pulaIntro = false;
int BUZZER = EEPROM.read(adress_configSound); //Define o numero da porta digital do buzzer
//int BUZZER = 13;

void setup() {
  Serial.begin(9600);
  iniciaPortas(); //Chama a função que define as portas
  iniciaDisplay();  //Chama a função que define as portas do display
  aberturaJogo(); //Chama a função responsavel por fazer a introdução do jogo
  iniciaJogo();
}

//Função responsavel por definir o modo das portas do arduino
void iniciaPortas() {
  pinMode(LED_VERDE,OUTPUT);
  pinMode(LED_AMARELO,OUTPUT);
  pinMode(LED_VERMELHO,OUTPUT);
  pinMode(LED_AZUL,OUTPUT);

  pinMode(BOTAO_VERDE,INPUT_PULLUP);
  pinMode(BOTAO_AMARELO,INPUT_PULLUP);
  pinMode(BOTAO_VERMELHO,INPUT_PULLUP);
  pinMode(BOTAO_AZUL,INPUT_PULLUP);

  pinMode(BUZZER,OUTPUT);
  }

//Função responsavel por iniciar o display
void iniciaDisplay()  {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

//Função que faz a introduçao gráfica do jogo
void aberturaJogo(){
    Serial.println("abertura do jogo");
    lcd.clear();
    for(int i=0; i < 16; i++){
      skipIntro();
      if(pulaIntro == false){
        lcd.setCursor(i,0);
        lcd.print("-");
        delay(200);
        if(i==2){
          lcd.setCursor(1,1);
          lcd.print("Seja bem vindo");
        }else if(i==9) {
          lcd.setCursor(0,1);
          lcd.print("                ");
          lcd.setCursor(2,1);
          lcd.print("Vai comecar"); 
        }    
      }
    }
    if(pulaIntro == false){
      tocaSom(600);
      piscaLed2(LED_AZUL);
      tocaSom(800);
      piscaLed2(LED_VERMELHO);
      tocaSom(400);
      piscaLed2(LED_AMARELO);
      tocaSom(1000);
      piscaLed2(LED_VERDE);
      lcd.clear();
      delay(1000);
      }
    lcd.clear(); 
  }

//Função principal que gera os numeros randomicos e mostra os dados na tela
void iniciaJogo() {
  int jogo = analogRead(0);
  randomSeed(jogo);
  for(int indice = 0; indice < TAMANHO_SEQUENCIA; indice ++) {
    sequenciaLuzes[indice] = sorteiaCor();
    }
  lcd.print("SEUS PONTOS:");//12 COLUNAS USADAS
  lcd.setCursor(13,0);
  lcd.print(pontos);
  lcd.setCursor(0,1);
  lcd.print("RECORDE:");//8 COLUNAS USADAS
  lcd.setCursor(9,1);
  lcd.print(EEPROM.read(adress_SCORE));
}

//Variavel responsavel por dar ao sorteador os valores correspondentes aos pinos dos leds
int sorteiaCor() {
  return random(LED_VERDE, LED_AZUL + 1);
}
  
void loop() {
  if(pontos > EEPROM.read(adress_SCORE)){   //Condição responsavel por testar a todo momento
  Serial.println("recorde modificado");     //se o recorde necessita ser atualizado
  recorde = pontos;
  EEPROM.write(adress_SCORE, recorde);
  }
  recordReset();
  
  switch(estadoAtual()){ //Responsavel por fazer testes e chamar as funçoes coreespondentes a cada fase do jogo
    case PRONTO_PARA_PROX_RODADA: 
      //executa funções próxima rodada
      Serial.println("pronto para proxima rodada");
      delay(500);
      preparaNovaRodada();
      break;
    case USUARIO_RESPONDENDO:
      Serial.println("usuario respondendo");
      processaRepostaUsuario();
      break;
    case JOGO_FINALIZADO_SUCESSO:
      jogoFinalizadoSucesso();
      Serial.println("jogo finalizado com sucesso");
      break;
    case JOGO_FINALIZADO_FALHA:
      jogoFinalizadoFalha();
      tocaSom(300);
      Serial.println("jogo finalizado com falha");
      break;
  }
}

//Função que reseta o numero de leds respondidos e aumenta uma rodada e um led a sequencia
void preparaNovaRodada() {
  rodada++;
  leds_respondidos = 0;
  if(rodada <= TAMANHO_SEQUENCIA) {
    tocaLedsRodada();
  }
}

//Função que testa se o usuario respondeu corretamente e acrescenta pontos ao score caso o usuario acerte
void processaRepostaUsuario() { 
  int resposta = checaRespostaJogador();

  if(resposta == INDEFINIDO){
    return;
  }
  
  if(resposta == sequenciaLuzes[leds_respondidos]){
    leds_respondidos++;
    pontos++;
    lcd.setCursor(13,0);
    lcd.print(pontos);
    Serial.println("reposta certa");
  }else {
    Serial.println("reposta errada");
    rodada = TAMANHO_SEQUENCIA + 2;
  }
}

//Função que testa de fato qual o estado do jogo
int estadoAtual() {
  if(rodada <= TAMANHO_SEQUENCIA){
    if(leds_respondidos == rodada){
        return PRONTO_PARA_PROX_RODADA;
      }else {
        return USUARIO_RESPONDENDO;
      }  
  }else {
    if(rodada == TAMANHO_SEQUENCIA + 1){
      return JOGO_FINALIZADO_SUCESSO;
    }else {
      return JOGO_FINALIZADO_FALHA;  
    }
  }
}

//Função responsavel por piscar os leds sorteados na funçao iniciaJogo()
void tocaLedsRodada() {
    for(int indice= 0; indice < rodada; indice ++) {
      piscaLed(sequenciaLuzes[indice]);
  }
}

//Função que testa todos os botões e envia as informaçoes para a funçao processaRepostaUsuario()
int checaRespostaJogador() {
  if(digitalRead(BOTAO_VERDE) == LOW) {
     return piscaLed(LED_VERDE);  
  }
  if(digitalRead(BOTAO_AMARELO) == LOW) {
     return piscaLed(LED_AMARELO);  
  }
  if(digitalRead(BOTAO_VERMELHO) == LOW) {
     return piscaLed(LED_VERMELHO);  
  }
  if(digitalRead(BOTAO_AZUL) == LOW) {
     return piscaLed(LED_AZUL);  
  }
  return INDEFINIDO;
} 

//AS FUNÇÕES ABAIXO SÃO PARA EXECUTAR UM FEEDBACK VISUAL AO USUARIO
void jogoFinalizadoSucesso() {
  EEPROM.write(adress_SCORE, 0x00);
  resetButton();
  ledWinner();
  displayWinner();
  piscaSequencia2();
  tocaSom(2800);
  piscaSequencia2();
  tocaSom(2800);
  piscaSequencia2();
}

void jogoFinalizadoFalha() {
  recordReset();
  resetButton();
  ledGameover();
  displayNovoRecorde();
  piscaSequencia2();
}

void piscaSequencia1() {
  piscaLed(LED_VERDE);
  piscaLed(LED_AMARELO);
  piscaLed(LED_VERMELHO);
  piscaLed(LED_AZUL);
  delay(500);
  }

void piscaSequencia2() {
  digitalWrite(LED_VERDE,HIGH);
  digitalWrite(LED_AMARELO,HIGH);
  digitalWrite(LED_VERMELHO,HIGH);
  digitalWrite(LED_AZUL,HIGH);
  delay(500);
  digitalWrite(LED_VERDE,LOW);
  digitalWrite(LED_AMARELO,LOW);
  digitalWrite(LED_VERMELHO,LOW);
  digitalWrite(LED_AZUL,LOW);
  delay(500);
  }

int piscaLed(int portaLed) {

  verificaSomLed(portaLed);
  
  digitalWrite(portaLed,HIGH);
  delay(500);
  digitalWrite(portaLed,LOW);
  delay(500);
  return portaLed;
  }
    
int piscaLed2(int portaLed) { //UTILIZADO NA ABERTURA DO JOGO
  digitalWrite(portaLed,HIGH);
  delay(150);
  digitalWrite(portaLed,LOW);
  return portaLed;
  }

void ledGameover() {
  for(int i = 0; i < 2; i++) {
    digitalWrite(LED_VERMELHO,HIGH);
    delay(500);
    digitalWrite(LED_VERMELHO,LOW);
    tocaSom(800);
    delay(250);
    tocaSom(600);
    delay(250);
    tocaSom(1800);
    delay(250);
    tocaSom(1000);
  }
}

void ledWinner() {
  for(int i = 0; i < 2; i++) {
    digitalWrite(LED_VERDE,HIGH);
    delay(500);
    digitalWrite(LED_VERDE,LOW);
    tocaSom(2000);
    delay(250);
    tocaSom(2600);
    delay(250);
    tocaSom(2800);
    delay(250);
    tocaSom(2400);
  }
}

//MENSAGENS DO DISPLAY
void displayWinner(){
  lcd.clear();
  for(int i=0; i<16; i++){
    lcd.setCursor(i,0);
    lcd.print("\\O/");
    delay(100);
  }
  lcd.setCursor(4,0);
  lcd.print(" WINNER ");
  piscaLed(LED_VERDE);
  resetButton();
  delay(500);
  lcd.clear();
  delay(250);
  for(int i=0; i<16; i++){
  lcd.setCursor(i,0);
  lcd.print("\\O/");
  }
  lcd.setCursor(4,0);
  lcd.print(" WINNER ");
  piscaLed(LED_VERDE);
  delay(500);
  lcd.clear();
  delay(500);
  lcd.print("VOCE TERMINOU");
  lcd.setCursor(5,1);
  lcd.print("O JOGO !!!");
  delay(2000);
  lcd.clear();
  lcd.print("O RECORDE IRA  ");
  lcd.setCursor(5,1);
  lcd.print("REINICIAR");
}

void displayGameover(){
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("GAMEOVER");
  delay(500);
  tocaSom(800);
  delay(1500);
  resetButton();
  lcd.clear();
  lcd.print("VOCE FEZ");
  lcd.setCursor(10,0);
  lcd.print(pontos);
  lcd.setCursor(0,1);
  lcd.print("PONTOS");
  delay(1500);
  resetButton();
  lcd.clear();
  lcd.print("RECORDE ATUAL:");
  lcd.setCursor(9,1);
  lcd.print(EEPROM.read(adress_SCORE));
}

void displayNovoRecorde(){
  if(pontos == EEPROM.read(adress_SCORE)){
    Serial.println("novo recorde instaurado");
    lcd.clear();
    delay(150);
    lcd.print("PARABENS \\O/\\O/");
    piscaLed(LED_VERDE);
    piscaLed(LED_VERDE);
    resetButton();
    delay(1000);
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("NOVO RECORDE");
    delay(2000);
    resetButton();
    lcd.clear();
    lcd.print("VOCE FEZ:");
    lcd.setCursor(0,1);
    lcd.print(EEPROM.read(adress_SCORE));
    lcd.setCursor(4,1);
    lcd.print("PONTOS");
  }else displayGameover();
}

void skipIntro(){
  if(digitalRead(BOTAO_AZUL) == LOW){
    pulaIntro = true;
    iniciaJogo();
  }
  if(digitalRead(BOTAO_VERMELHO) == LOW){
    pulaIntro = true;
    iniciaJogo();
  }
  if(digitalRead(BOTAO_AMARELO) == LOW){
    pulaIntro = true;
    iniciaJogo();
  }
  if(digitalRead(BOTAO_VERDE) == LOW){
    pulaIntro = true;
    iniciaJogo();
  }
      
  }

//REINICIADOR DO ARUDINO
void(*funcReset)() = 0;
void resetButton(){
  muteSound();
  for(int i=0; i<5; i++){
    delay(150);
    if(digitalRead(BOTAO_AZUL) == LOW){
    delay(250);
      if(digitalRead(BOTAO_AZUL) == LOW){
        delay(1500);
        if(digitalRead(BOTAO_AZUL) == LOW){
          lcd.clear();
          lcd.print("REINICIANDO...");
          delay(2000);
          Serial.println("reiniciando...");
          delay(250);
          funcReset();
        }
      }
    }else break;
  }
}

//RESET DE SCORE
void recordReset(){
  muteSound();  //chama a funcao muteSound para nao repeti-la durante todo o codigo, como feito pela funcao recordReset
  if(digitalRead(BOTAO_VERDE) == LOW and digitalRead(BOTAO_VERMELHO) == LOW){
    EEPROM.write(adress_SCORE, 0x00);
    tocaSom(1500);
    Serial.println("o dispositivo sera reiniciado em breve");
    lcd.clear();
    lcd.print("O RECORDE FOI");
    lcd.setCursor(4,1);
    lcd.print("REINICIADO");
    delay(2000);
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("POR FAVOR");
    lcd.setCursor(0,1);
    lcd.print("REINICIE O JOGO!");
    for(int i=0;i<20;i++){
      resetButton();
      delay(500);
      }
    Serial.println("reiniciando...");
    delay(250);
    funcReset();
  }
}

//CONFIGURAÇÕES DE SOM
void tocaSom(int frequencia) {
  tone(BUZZER, frequencia, 100);
}

void verificaSomLed(int portaLed) {
  switch (portaLed) {
    case LED_VERDE:
      tocaSom(2000);
      break;
    case LED_AMARELO:
      tocaSom(2200);
      break;
    case LED_VERMELHO:
      tocaSom(2400);
      break;
    case LED_AZUL:
      tocaSom(2500);
      break;
  }
}

void muteSound(){
  for(int i=0; i<5; i++){
    if(digitalRead(BOTAO_AMARELO) == LOW){
    delay(250);
      if(digitalRead(BOTAO_AMARELO) == LOW){
        delay(1500);
        if(digitalRead(BOTAO_AMARELO) == LOW){
          if(EEPROM.read(som) == true){
              EEPROM.write(adress_configSound,-10);
              BUZZER = EEPROM.read(adress_configSound);
              EEPROM.write(som, false);
              lcd.clear();
              lcd.print("SILENCIADO...");
              Serial.println("som desativado");
              delay(2000);
              lcd.clear();
              lcd.print("REINICIANDO...");
              delay(750);
              Serial.println("reiniciando...");
              delay(250);
              funcReset();
            }
           if(EEPROM.read(som) == false){
            EEPROM.write(adress_configSound,13);
            BUZZER = EEPROM.read(adress_configSound);
            EEPROM.write(som, true); 
            lcd.clear();
            lcd.print("SOM ATIVO...");
            Serial.println("som ativo");
            delay(2000);
            lcd.clear();
            lcd.print("REINICIANDO...");
            delay(750);
            Serial.println("reiniciando...");
            delay(250);
            funcReset();
           }
        }
      }
    }
  }
}
