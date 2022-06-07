#include <stdio.h>
#include <stdlib.h>
#define F_CPU 16000000UL /* Define CPU Frequency e.g. here 16MHz */
#include <avr/io.h> /* Include AVR std. library file */
#include <util/delay.h> /* Include Delay header file */
#include <avr/interrupt.h>
#define LCD_Dir  DDRD /* Define LCD data port direction */
#define LCD_Port PORTD /* Define LCD data port */
#define RS PD2 /* Define Register Select pin */
#define EN PD3  /* Define Enable signal pin */
//Inicio y final del juego
int time_count;
int bandera = 1;
int seg = 0;
int GameStart = 0;
//Diseño y coordenadas de los personajes
unsigned char coorUP[] = {7,8,12,16,17,19,25,31,33,37};//10
unsigned char coorDO[] = {68,69,74,78,85,87,91,93,99,103};//10
unsigned char Pattern1[]={0x0E,0x15,0x1F,0x11,0x15,0x1F,0x1F,0x15}; //Obstaculo1
unsigned char Pattern2[]= {0x07,0x0F,0x11,0x1E,0x18,0x1F,0x0F,0x07}; //PersonajE
//Posiciones de personaje y obstaculos
char Puntaje[3];
int ptj = 0;
int value;
int filaPer = 0;
int posPer = 0;
//Sonido
int nota;
int Stime_count;
int SBandera = 0;
int nNotas = 0;
int fase = 0;
int E = 30;
int arcade[]={38,0,30,0,26,23,26, 38,0,30,0,26,23,26, 38,0,30,0,26,23,26, 38,0,30,0,26,23,26}; //El cero simula un silencio
int fase1[]={16,4,10,4,6,6,6, 16,4,10,4,6,6,6, 16,4,10,4,6,6,6, 16,4,10,4,6,6,6};
int fase2[]={8,2,5,2,3,3,3, 8,2,5,2,3,3,3, 8,2,5,2,3,3,3, 8,2,5,2,3,3,3};
int fase3[]={4,1,2,1,2,2,2, 4,1,2,1,2,2,2, 4,1,2,1,2,2,2, 4,1,2,1,2,2,2};

void CreateCustomCharacter (unsigned char *Pattern, const char Location)
{
int i=0;
LCD_Command (0x40+(Location*8));     //Send the Address of CGRAM
for (i=0; i<8; i++)
LCD_Char (Pattern [ i ] );         //Pass the bytes of pattern on LCD
}

void ADC_Init()
{
DDRC=0x0; /* Make ADC port as input ATmega328p pag 206*/
ADCSRA = 0x87; /* Enable ADC, fr/128  ATmega328p pag 218*/
ADMUX = 0x40; /* Vref: Avcc, ADC channel: 0 ATmega328p pag 
217*/
}
int ADC_Read(char channel)
{
int Ain,AinLow;
ADMUX=ADMUX|(channel & 0x0f); /* Set input channel to read */
ADCSRA |= (1<<ADSC); /* Start conversion */
while((ADCSRA&(1<<ADIF))==0); /* Monitor end of conversion interrupt */
_delay_us(10);
AinLow = (int)ADCL; /* Read lower byte ATmega328p pag 219*/
Ain = (int)ADCH*256; /* Read higher 2 bits and
Multiply with weight */
Ain = Ain + AinLow;
return(Ain); /* Return digital value*/
}
/* LCD.pdf pag 21*/
void LCD_Command( unsigned char cmnd )//Necesario para mover la camara*
{
LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); /* sending upper nibble */
LCD_Port &= ~ (1<<RS); /* RS=0, command reg. */
LCD_Port |= (1<<EN); /* Enable pulse */
_delay_us(10);
LCD_Port &= ~ (1<<EN);
_delay_us(200);
LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  /* sending lower nibble */
LCD_Port |= (1<<EN);
_delay_us(10);
LCD_Port &= ~ (1<<EN);
_delay_ms(20);
}

void LCD_Char( unsigned char data )
{
LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); /* sending upper nibble */
LCD_Port |= (1<<RS); /* RS=1, data reg. */
LCD_Port|= (1<<EN);
_delay_us(10);
LCD_Port &= ~ (1<<EN);
_delay_us(200);
LCD_Port = (LCD_Port & 0x0F) | (data << 4); /* sending lower nibble */
LCD_Port |= (1<<EN);
_delay_us(10);
LCD_Port &= ~ (1<<EN);
_delay_ms(20);
}

void LCD_Init (void) /* LCD Initialize function */
{
LCD_Dir = 0xFF; /* Make LCD port direction as o/p */
_delay_ms(20); /* LCD Power ON delay always >15ms */
CreateCustomCharacter(Pattern1,1);//Obstaculo - LCD_Char(1)
CreateCustomCharacter(Pattern2,2);//Moon-man - LCD_Char(2)
LCD_Command(0x02); /* send for 4 bit initialization of LCD  */
LCD_Command(0x28);              /* 2 line, 5*7 matrix in 4-bit mode */
LCD_Command(0x0c);              /* Display on cursor off*/
LCD_Command(0x06);              /* Increment cursor (shift cursor to right)*/
LCD_Command(0x01);              /* Clear display screen*/
_delay_ms(2);
}
void LCD_String (char str) / Send string to LCD function */
{
    int i;
    for(i=0;str[i]!=0;i++) /* Send each char of string till the NULL */
    {
        LCD_Char (str[i]);
    }
}

void LCD_String_xy (char row, char pos) {
if (row == 0 && pos<40){
    LCD_Command((pos & 0x3F)|0x80); /* Command of first row and required position<16 */
}
else if (row == 1 && pos<104){
    LCD_Command((pos & 0x3F)|0xC0); /* Command of first row and required position<16 */
   }
}

void LCD_Clear()
{
LCD_Command (0x01); /* Clear display */
_delay_ms(2);
LCD_Command (0x80); /* Cursor at home position */
}

ISR (TIMER0_OVF_vect) {
    TCNT0 = 96;                                                                 // Set for 10 us timeout
    TIFR0 = 1<<TOV0;
    if (GameStart == 1) { //Si GameStart es igual a cero, no correra el juego
        if (seg < 130) { 
            ++Stime_count;
            //Sonido, sacado de la practica4
            if (Stime_count == nota && SBandera == 0) {
                PORTB |= (1<<PINB1);                                                    
                Stime_count = 0;         
                SBandera = 1;
            } 
            if (Stime_count == nota && SBandera == 1) {
                PORTB &= ~(1<<PINB1);                                                   
                Stime_count = 0;         
                SBandera = 0;
            }
            ++time_count;
            //Movimiento
            // Primera fase: Velocidad normal
            if (seg < 30 && time_count == 30000 && bandera == 0) { 
                if (posPer < 39){
                    posPer++;
                }
                else {
                    posPer = 0;
                }
                LCD_String_xy(filaPer, posPer);
                LCD_Char(1);//Imprime al personaje principal
            
                ++seg;
                for (int i = 0; i < 10; i++) {
                    if (posPer == coorUP[i] && filaPer == 0){
                        ++ptj;
                    }
                    if ((posPer + 64) == coorDO[i] && filaPer == 1){
                        ++ptj;
                    }
                }
                fase = 0;
                time_count = 0;       
                bandera = 1;
                //Movimiento de la cámara
                LCD_Command(0x18);
            }
            if (seg < 30 && time_count == 30000 && bandera == 1) { 
                int choque = 0;
                int i = 0;
                while (i < 10 && choque == 0) {
                    if (posPer == coorUP[i] && filaPer == 0) {
                        LCD_String_xy(filaPer, posPer);
                        LCD_Char(2);//En caso de choque, vuelve a imprimir el obstaculo
                        choque = 1;
                    }
                
                    if ((posPer + 64) == coorDO[i] && filaPer == 1) {
                        LCD_String_xy(filaPer, posPer);
                        LCD_Char(2);
                        choque = 1;
                    }
                    ++i;
                } if (choque == 0) {
                    LCD_String_xy(filaPer, posPer);
                    LCD_String(" ");
                }
                fase = 0;
                time_count = 0;         
                bandera = 0;
            }
            //Fase 2: velocidad mediana
            if ((seg >= 30 && seg < 90) && time_count == 15000 && bandera == 0) {
                if (posPer < 39){
                    ++posPer;
                }
                else{
                    posPer = 0;
                }
                LCD_String_xy(filaPer, posPer);
                LCD_Char(1);
            
                ++seg;
                for (int i = 0; i < 10; i++) {
                    if (posPer == coorUP[i] && filaPer == 0) ptj += 2;
                    if ((posPer + 64) == coorDO[i] && filaPer == 1) ptj += 2;
                }
                fase = 1;
                time_count = 0;       
                bandera = 1;
                //Movimiento de la cámara
                LCD_Command(0x18);
            }
            if ((seg >= 30 && seg < 90) && time_count == 15000 && bandera == 1) {
                int choque = 0;
                int i = 0;
                while (i < 10 && choque == 0) {
                    if (posPer == coorUP[i] && filaPer == 0) {
                        LCD_String_xy(filaPer, posPer);
                        LCD_Char(2);
                        choque = 1;
                    }
                
                    if ((posPer + 64) == coorDO[i] && filaPer == 1) {
                        LCD_String_xy(filaPer, posPer);
                        LCD_Char(2);
                        choque = 1;
                    }
                    ++i;
                }
            
                if (choque == 0) {
                    LCD_String_xy(filaPer, posPer);
                    LCD_String(" ");
                }
            
                fase = 1;
                time_count = 0;         
                bandera = 0;
            }
            //Fase 3: velocidad rapida
            if ( (seg >= 90 && seg < 130) && time_count == 7500 && bandera == 0) {
                if (posPer < 39) ++posPer;
                else posPer = 0;
                LCD_String_xy(filaPer, posPer);
                LCD_Char(1);
            
                ++seg;
                for (int i = 0; i < 10; i++) {
                    if (posPer == coorUP[i] && filaPer == 0) ptj += 3;
                    if ((posPer + 64) == coorDO[i] && filaPer == 1) ptj += 3;
                }
                //Movimiento de la cámara
                LCD_Command(0x18);
                
                fase = 2;
                time_count = 0;       
                bandera = 1;
            }
            if ((seg >= 90 && seg < 130) && time_count == 7500 && bandera == 1) {
                int i = 0;
                int choque = 0;
                while (i < 10 && choque == 0) {
                    if (posPer == coorUP[i] && filaPer == 0) {
                        LCD_String_xy(filaPer, posPer);
                        LCD_Char(2);
                        choque = 1;
                    }
                
                    if ((posPer + 64) == coorDO[i] && filaPer == 1) {
                        LCD_String_xy(filaPer, posPer);
                        LCD_Char(2);
                        choque = 1;
                    }
                    ++i;
                }
                if (choque == 0) {
                    LCD_String_xy(filaPer, posPer);
                    LCD_String(" ");
                }
                bandera = 0;
                fase = 2;
                time_count = 0;         
            }
        } else {                                                                //Al llegar a los noventa segundos, se reinicia todos 
                                                                                //los parametros
            GameStart = 0;
            PORTB &= ~(1<<PINB1); 
            LCD_Clear();
            LCD_String("Game Over!!");
            _delay_ms(500);
            LCD_Clear();
            LCD_String("Le hice este ");
            LCD_Command(0xC0);
            LCD_String("proyecto con ");
            _delay_ms(1000);
            LCD_Clear();
            LCD_String("mis lagrimas");
            LCD_Command(0xC0);
            LCD_String("profe :(");
            _delay_ms(1000);
            LCD_Clear();
            //Con la siguiente variable, convertimos un valor int a char. De esta manera, podremos guardarlo en la variable Puntaje e imprimirlo
            itoa(180 - ptj, Puntaje, 10);                                     
            LCD_String("Puntaje: ");
            //LCD_Command(0xC0);
            LCD_String_xy(0, 10);
            LCD_String(Puntaje);
            _delay_ms(3000);
            LCD_Clear();
            //Reiniciamos todo
            reinicio();
            Intro();
            obstaculos();
            LCD_String_xy(0, 0);
            LCD_Char(1);
            _delay_ms(250);
            LCD_String_xy(0, 0);
            LCD_String(" ");
            LCD_Char(1);
            ++posPer;
            _delay_ms(500);
            GameStart = 1;
        }
    }
}

void personaje(){
    ADC_Init();
        value = ADC_Read(0);
        if (value < 512) { 
            if (filaPer != 0) {
                filaPer = 0;
                LCD_String_xy(1, posPer);
                LCD_String(" ");//Con esto, sobrescribes encima del personaje. Así, no tendrás que eliminar todo en el tablero.
            }
        } else {
            if (filaPer != 1) {
                filaPer = 1;
                LCD_String_xy(0, posPer);
                LCD_String(" ");
            }
        }
}

void obstaculos(){
    for(unsigned char i = 0; i < 10; i++) {
        LCD_String_xy(0, coorUP[i]);
        LCD_Char(2);
        LCD_String_xy(1, coorDO[i]);
        LCD_Char(2);
    }
}
void tempo(unsigned char duracion) {
    while(duracion > 0) {
        duracion--;
        _delay_ms(1);
    }
}

void reinicio (){
            nNotas = 0;
            fase = 0;
            SBandera = 0;
            Stime_count = 0;
            seg = 0;
            bandera = 1;
            time_count = 0;
            ptj = 0;
            posPer = 0;
            filaPer = 0;
}

void C4(){
    return nota = 38;
}
void E4(){
    return nota = 30;
}
void G4(){
    return nota = 26;
}
void A4(){
    return nota = 23;
}

void silencio(int z) {
    PORTB = 0x00;                                                                
    Stime_count = 0;         
    SBandera = 1;
    tempo(E);   
}

void Intro(){
    LCD_String("Tecnológico de ");
    LCD_Command(0xC0);
    LCD_String("   Monterrey   ");
    _delay_ms(1000);
    LCD_Clear();
    LCD_String("   Equipo 3   ");
    LCD_Command(0xC0);
    LCD_String("   Presenta   ");
    _delay_ms(1000);
    LCD_Clear();
    LCD_String("   Ghost-man   ");
    _delay_ms(1000);
    LCD_Clear(); 
    LCD_String("   START!!!   ");
    _delay_ms(500);
    LCD_Clear(); 
}
void song(){
        if(nNotas < 28) {
            //Dependiendo en la velocidad en que se encuentre, la melodia será reproducida
            //Fase 1
            if (fase == 0) { 
                nota = arcade[nNotas];
                tempo(fase1[nNotas]);               
                E = 30;
                silencio(nNotas);
                ++nNotas;
            } 
            //Fase 2
            else if (fase == 1) {                                            
                nota = arcade[nNotas];
                tempo(fase2[nNotas]);               
                E = 20;
                silencio(nNotas);
                ++nNotas;
            } 
            //Fase 3
            else {                                                            
                nota = arcade[nNotas];
                tempo(fase3[nNotas]);               
                E = 10;
                silencio(nNotas);
                ++nNotas;
            }
        } else nNotas = 0;   
}

int main() {
    cli();
    DDRB = 0b00000010;                                                                
    TCCR0B = 0x01; //Ajustamos el prescaler en 8
    TCNT0 = 0x00;
    TIFR0 = 1<<TOV0;
    TIMSK0 = 1<<TOIE0;
    sei();
    //Inicializamos el ADC, LCD, el intro del juego y escribimos los obstaculos
	ADC_Init();
	LCD_Init();                                                                 
    Intro();
    obstaculos();
    LCD_String_xy(0, 0);
    LCD_Char(1);
    _delay_ms(100);
    LCD_String_xy(0, 0);
    LCD_String(" ");
    LCD_Char(1);
    posPer++;
    
    //El cursos dejará de verse
    LCD_Command(0x0C);
    _delay_ms(500);
    //Iniciamos el juego (Es decir, el Timer0)
    GameStart = 1;
    
	while(1) {
        song();
        GameStart = 0;
        personaje();
        GameStart = 1;
	}
}
