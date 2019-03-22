#include <Ucglib.h>
#include < SPI.h > 


const int FQ_UD = 11; //A1)DDS Pin Assign JA2GQP
const int SDAT = 10; //zzz
const int SCLK = 12; //
const int RESET = 9; //

double prevs[120]; //Array für SWR

double _lowestSwrFrq = 0; // Variable für Freq. mit SWR Min.
double _lowestSwr = 0; // Variable für SWR Min.

double _frqLow = 1; // Freq. Links unterste Zeile Display
double _frqMid = 15; // Freq. Mitte unterste Zeile Display
double _frqHigh = 30; // Freq. Mitte unterste Zeile Display

long _mils2 = 0; //Millisekunden für Entprellung Interrupt
long _mils = 0; //Millisekunden für Entprellung Interrupt

double _frqStart = 1; // Start Frequency for sweep
double _frqStop = 30; // Stop Frequency for sweep
double _currFrq; // Temp variable used during sweep

Ucglib_ILI9341_18x240x320_SWSPI ucg(8, 7, 6, 5, 4); // sclk,data,cd,cs,reset 


void setup() {

    ucg.begin(UCG_FONT_MODE_SOLID);

    ucg.setRotate90();
    ucg.setFont(ucg_font_7x14_mf);

    pinMode(FQ_UD, OUTPUT);
    pinMode(SCLK, OUTPUT);
    pinMode(SDAT, OUTPUT);
    pinMode(RESET, OUTPUT);

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    attachInterrupt(0, button1, FALLING);
    
    _mils2 = millis();

    pinMode(3, OUTPUT);
    digitalWrite(3, HIGH);
    attachInterrupt(1, button2, FALLING);

    // Configure LED pin for digital output
    pinMode(13, OUTPUT);

    pinMode(A0, INPUT); // Set up analog inputs on A0 and A1
    pinMode(A1, INPUT);
    analogReference(INTERNAL); //internal reference voltage

    Serial.begin(57600); // initialize serial communication at 57600 baud

    DDS_init();

    ucg.clearScreen();
    drawGrid();
    printLabels();
    refreshValues();
    refreshScale();
}

void loop() {
    Perform_sweep();
}



int _gridXMin = 17;  
int _gridXMax = 319;  

int _gridYMin = 19;
int _gridYMax = 222;

int _gridWidth = _gridXMax - _gridXMin;
int _gridHeight = _gridYMax - _gridYMin;

int _raster = 5;
int _steps = _gridWidth / _raster; 

double scaleY(double val){
  return _gridYMax - val * _gridHeight / 10;
}

void Perform_sweep() {
    double frqStep = (_frqStop - _frqStart) / _steps;
    double prev = 10;
    double prev2 = 10;
    double fix = 0;
    double swr;
    
    drawGrid();
    
    _lowestSwr = 10;
    
    int X = _gridXMin + 1;
    int Y = 1;
    int Y2 = 1;
    
    for (int i = 0; i < _steps; i++) {
        _currFrq = _frqStart + frqStep * i;

        DDS_SetFrq(_currFrq);

        swr = checkSWR();
        
        if (swr < _lowestSwr && swr > 1) { //Minimum SWR
            _lowestSwr = swr; //Minimum SWR and frequency store
            _lowestSwrFrq = _currFrq;
        }

        ucg.setColor(0, 0, 0);
        Y = scaleY(prev2);
        Y2 = scaleY(prevs[i]);
        drawLine(X, Y, X + _raster, Y2);
        prev2 = prevs[i];


        ucg.setColor(255, 255, 0);
        Y = scaleY(prev);
        Y2 = scaleY(swr);
        drawLine(X, Y, X + _raster, Y2);
        prev = fix = swr;

        X += _raster;

        
        prevs[i] = swr;

        Serial.println(swr);
    }

    refreshValues();
}

void drawLine(int x1, int y1, int x2, int y2) {
    ucg.drawLine(x1, y1, x2, y2);
}


void prepareDisplay() {
    ucg.clearScreen();
    drawGrid();
    printLabels();
    refreshValues();
    refreshScale();
}

void drawGrid() {
    ucg.setColor(200, 200, 200);
    
    // poziome
    int y1 = _gridYMax - _gridHeight / 10 * 1;
    int y2 = _gridYMax - _gridHeight / 10 * 2;
    int y3 = _gridYMax - _gridHeight / 10 * 3;
    int y5 = _gridYMax - _gridHeight / 10 * 5;

    ucg.drawHLine(_gridXMin, y1, _gridWidth); //swr 1
    ucg.drawHLine(_gridXMin, y2, _gridWidth); //swr 2
    ucg.drawHLine(_gridXMin, y3, _gridWidth); //swr 3
    ucg.drawHLine(_gridXMin, y5, _gridWidth); //swr 5

    // pionowe
    int xSpace = _gridWidth/4;
    int x1 = _gridXMin + xSpace;
    int x2 = _gridXMin + xSpace * 2;
    int x3 = _gridXMin + xSpace * 3;
    
    ucg.drawVLine(x1, _gridYMin, _gridHeight);
    ucg.drawVLine(x2, _gridYMin, _gridHeight);
    ucg.drawVLine(x3, _gridYMin, _gridHeight);
    // ramka
    ucg.drawRFrame(_gridXMin, _gridYMin, _gridWidth, _gridHeight, 1);
}

void printLabels() {
    // nad wykresem
    int vShift = 15;
    ucg.setColor(220, 220, 220);
    ucg.setPrintPos(26, vShift); ucg.print("MIN SWR 1:");
    ucg.setPrintPos(200, vShift); ucg.print("@");
    ucg.setPrintPos(247, vShift); ucg.print("MHz");
    
    
    // z lewej strony
    int y1 = _gridYMax - _gridHeight / 10 * 1;
    int y2 = _gridYMax - _gridHeight / 10 * 2;
    int y3 = _gridYMax - _gridHeight / 10 * 3;
    int y5 = _gridYMax - _gridHeight / 10 * 5;
    
    ucg.setPrintPos(_gridXMin - 10, y1 + 5); ucg.print("1");
    ucg.setPrintPos(_gridXMin - 10, y2 + 5); ucg.print("2");
    ucg.setPrintPos(_gridXMin - 10, y3 + 5); ucg.print("3");
    ucg.setPrintPos(_gridXMin - 10, y5 + 5); ucg.print("5");
    ucg.setPrintPos(_gridXMin - 16, _gridYMin + 5); ucg.print("10");
}

void refreshValues() {
    
    // nad wykresem
    int vShift = 15;
    ucg.setColor(255, 255, 0);

    ucg.setPrintPos(98, vShift);
    ucg.print(_lowestSwr, 2);

    ucg.setColor(255, 255, 0);
    ucg.setPrintPos(209, vShift);
    ucg.print(_lowestSwrFrq, _lowestSwrFrq < 10 ? 3 : 2);
}

void refreshScale() {
    ucg.setColor(220, 220, 220);
    int vSpace = 14;

    ucg.setPrintPos(_gridXMin, _gridYMax + vSpace);
    ucg.print(_frqLow, _frqLow < 10 ? 2 : 1);

    ucg.setPrintPos((_gridXMax+_gridXMin)/2-13,  _gridYMax + vSpace);
    ucg.print(_frqMid, _frqMid < 10 ? 2 : 1);

    ucg.setPrintPos(_gridXMax-28,  _gridYMax + vSpace);
    ucg.print(_frqHigh, (_frqHigh < 10 ? 2 : 1));
}

void button1() {
    _mils = millis();
    if (_mils - _mils2 < 1000) {
        _mils2 = _mils;
        return;
    } 
    _mils2 = _mils;

    _frqStart = 1; // Start Frequency for sweep
    _frqStop = 30; // Stop Frequency for sweep
    _frqLow = 1; // Unterste Zeile Display Freq. Links
    _frqMid = 15; // Unterste Zeile Display Freq. Mitte
    _frqHigh = 30;

    _lowestSwrFrq = 0;
    _lowestSwr = 0;

    ucg.clearScreen();
    drawGrid();
    printLabels();
    refreshValues();
    refreshScale();
}

void button2() {
    _mils = millis(); //Entprellen mit millis()
    if (_mils - _mils2 < 1000) {
        _mils2 = _mils;
        return;
    }
    _mils2 = _mils;

    int x = _lowestSwrFrq + 0.5; //Runde auf Mhz

    _frqStart = x - 1; // Start Frequency for sweep
    _frqStop = x + 1; // Stop Frequency for sweep

    _frqLow = x - 1; // Unterste Zeile Display Freq. Links
    _frqMid = x; // Unterste Zeile Display Freq. Mitte
    _frqHigh = x + 1; // Unterste Zeile Display Freq. Rechts

    _lowestSwrFrq = 0;
    _lowestSwr = 0;

    ucg.clearScreen();
    drawGrid();
    printLabels();
    refreshValues();
    refreshScale();
}

double checkSWR() {
    delay(2);
    double FWD = 0;
    double REF = 0;
    double SWR = 10;
    
    REF = analogRead(A0);
    FWD = analogRead(A1);

    REF -= 10; // Offset Correction
    REF = REF >= FWD ? FWD - 1 : REF;
    REF = REF < 1 ? 1 : REF;

    SWR = (FWD + REF) / (FWD - REF);

    return SWR > 10 ? 10 : SWR;
}

void DDS_init() {
    digitalWrite(RESET, HIGH); //A3)DDS Reset Sequence JA2GQP
    delay(1); //                            
    digitalWrite(RESET, LOW); //
    // 
    digitalWrite(SCLK, HIGH); //          
    digitalWrite(SCLK, LOW); //
    digitalWrite(FQ_UD, HIGH); //
    digitalWrite(FQ_UD, LOW); //
}

void DDS_SetFrq(double Freq_Hz) {
    int32_t f = Freq_Hz * 4294967295 / 125000000  * 1000000; // Calculate the DDS word - from AD9850 Datasheet
    for (int b = 0; b < 4; b++, f >>= 8) { // Send one byte at a time
        DDS_sendByte(f & 0xFF);
    }
    DDS_sendByte(0); // 5th byte needs to be zeros(AD9850 Command Parameters)                              
    digitalWrite(FQ_UD, HIGH); // Strobe the Update pin to tell DDS to use values
    digitalWrite(FQ_UD, LOW);
}

void DDS_sendByte(byte data_to_send) {
    for (int i = 0; i < 8; i++, data_to_send >>= 1) { // Bit bang the byte over the SPI bus
        digitalWrite(SDAT, data_to_send & 0x01); // Set Data bit on output pin
        digitalWrite(SCLK, HIGH); // Strobe the clock pin
        digitalWrite(SCLK, LOW);
    }
}
