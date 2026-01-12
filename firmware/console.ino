#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

// evitar errores
void f1DibujarHUD();
void f1DibujarPieza();
void dibujarCocheGrande(uint8_t cx, uint8_t cy, uint8_t piezas);
void f1LeerEntrada();
void f1AvanzarPaso();
void tenisAvanzarPaso();
void f1DibujarCeldaCoche(int cx, int cy, uint8_t piezas);
void dibujarRuedasUD(int px, int py, bool arriba, uint8_t piezas);
void dibujarRuedasLR(int px, int py, bool izquierda, uint8_t piezas);
void dibujarSoloPista();
void tenisDibujar();
void sfxPausa(bool encendido);

// evitar errores usar (Estela es para la estela del f1)
struct Estela;

// opcion aire sucio 0 no hay 1 si hay
#define ACTIVAR_ESTELA 1

// definir pantalla 
#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA 64
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);
bool oled_ok = false;

// definir botones
#define PIN_IZQ   2
#define PIN_DER   3
#define PIN_ABAJO 4
#define PIN_ARRIBA 5
#define PIN_OK    6
#define PIN_MENU  7

// definir zumbador
#define PIN_ZUMBADOR 8

// cuadricula juego f1
const uint8_t CELDA = 8;
const uint8_t COLUMNAS = ANCHO_PANTALLA  / CELDA;  // 16
const uint8_t FILAS    = ALTO_PANTALLA   / CELDA;  // 8

// estados
enum EstadoJuego {
  EST_SELEC_JUEGO,
  EST_F1_MENU, EST_F1_JUEGO, EST_F1_GARAJE, EST_F1_FIN,
  EST_TNS_MENU, EST_TNS_JUEGO, EST_TNS_PUNTO, EST_TNS_FIN,
  EST_CREDITOS
};
EstadoJuego estado = EST_SELEC_JUEGO;

// menu y creditos
uint8_t idxMenuPrincipal = 0;
uint8_t idxMenuF1   = 0;
uint8_t idxMenuTenis  = 0;
int16_t yCreditos = ALTO_PANTALLA;
unsigned long ultCreditos = 0;

// estructura botones y sistema antirrebote
struct Boton { uint8_t pin; bool crudo; bool estable; unsigned long tBorde; Boton(uint8_t p):pin(p),crudo(false),estable(false),tBorde(0){} };
Boton bArriba(PIN_ARRIBA), bAbajo(PIN_ABAJO), bIzq(PIN_IZQ), bDer(PIN_DER), bOK(PIN_OK), bMenu(PIN_MENU);
inline bool leerBruto(uint8_t pin){ return digitalRead(pin) == LOW; }
bool actualizarBoton(Boton &b, uint16_t reboteMs = 25) {
  bool r = leerBruto(b.pin);
  if (r != b.crudo) { b.crudo = r; b.tBorde = millis(); }
  if (millis() - b.tBorde >= reboteMs && b.estable != b.crudo) { b.estable = b.crudo; return true; }
  return false;
}

// sonidos
void sfxMoverMenu()     { tone(PIN_ZUMBADOR, 1500, 40); }
void sfxSeleccionarMenu()   { tone(PIN_ZUMBADOR, 2000, 90); }
void sfxRecoger()       { tone(PIN_ZUMBADOR, 1750, 35); delay(25); tone(PIN_ZUMBADOR, 1200, 35); }
void sfxPasoEnsamblaje() { tone(PIN_ZUMBADOR, 900, 40);  delay(15); tone(PIN_ZUMBADOR, 1300, 45); }
void sfxPausa(bool encendido) { tone(PIN_ZUMBADOR, encendido ? 800 : 1400, 80); }
void sfxChoque()        { for (int f=1200; f>=300; f-=40){ tone(PIN_ZUMBADOR,f,18); delay(12);} }
void sfxPong()         { tone(PIN_ZUMBADOR, 2200, 20); }
void sfxPared()         { tone(PIN_ZUMBADOR, 1200, 18); }
void sfxPuntuacion()        { tone(PIN_ZUMBADOR, 600, 180); }
void sfxSaque()        { tone(PIN_ZUMBADOR, 1700, 70); }

// texto basico
void dibujarTitulo(const __FlashStringHelper* t, uint8_t len, int y){
  int x = (ANCHO_PANTALLA - (int)len*12)/2; if (x<0) x=0;
  display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y); display.print(t);
}
int16_t anchoFlash(const __FlashStringHelper* s, uint8_t size){
  PGM_P p = reinterpret_cast<PGM_P>(s);
  size_t len = strlen_P(p);
  return (int16_t)(len * 6 * size);
}
void dibujarCentradoP(const __FlashStringHelper* s, int y, uint8_t size){
  int16_t w = anchoFlash(s, size);
  int16_t x = (ANCHO_PANTALLA - w) / 2; if (x < 0) x = 0;
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.print(s);
}
// textos
void dibujarCajaTexto(int x, int y, const char* txt, uint8_t size=1, uint8_t pad=1){
  int w = (int)strlen(txt) * 6 * size;
  int h = 8 * size;
  display.fillRect(x - pad, y, w + 2*pad, h, SSD1306_BLACK);
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, y);
  display.print(txt);
}
void dibujarCajaTextoCentrada(const char* txt, int y, uint8_t size=1, uint8_t pad=1){
  int w = (int)strlen(txt) * 6 * size;
  int x = (ANCHO_PANTALLA - w) / 2; if (x < 0) x = 0;
  dibujarCajaTexto(x, y, txt, size, pad);
}

// guardado de coches completados
const int EE_DIR_MAGIA   = 0;
const int EE_DIR_VERSION = 4;
const int EE_DIR_CONT_GARAJE = 5;
const uint32_t MAGIA = 0x31475246UL;
const uint8_t  VERSION = 1;
uint16_t conteo_garaje = 0;

void guardarGaraje(){ EEPROM.put(EE_DIR_MAGIA, MAGIA); EEPROM.update(EE_DIR_VERSION, VERSION); EEPROM.put(EE_DIR_CONT_GARAJE, conteo_garaje); }
void cargarGaraje(){
  uint32_t m=0; EEPROM.get(EE_DIR_MAGIA, m);
  uint8_t v=EEPROM.read(EE_DIR_VERSION);
  if (m==MAGIA && v==VERSION){ EEPROM.get(EE_DIR_CONT_GARAJE, conteo_garaje); }
  else { conteo_garaje = 0; guardarGaraje(); }
}

// juego f1
#define PARTE_NARIZ    0x01
#define PARTE_RUEDAS  0x02
#define PARTE_ALERON_DEL  0x04
#define PARTE_ALERON_TRAS   0x08
#define PARTE_PONTONES   0x10
#define PARTE_HALO    0x20
#define PARTE_TODO     (PARTE_NARIZ|PARTE_RUEDAS|PARTE_ALERON_DEL|PARTE_ALERON_TRAS|PARTE_PONTONES|PARTE_HALO)

uint8_t piezasRecolectadas = 0;

// iconos 8x8
const uint8_t PROGMEM ic_nariz[8]   = { 0x18,0x3C,0x7E,0x7E,0x3C,0x18,0x00,0x00 };
const uint8_t PROGMEM ic_ruedas[8] = { 0xC3,0xC3,0x00,0x3C,0x3C,0x00,0xC3,0xC3 };
const uint8_t PROGMEM ic_aleron_del[8]  = { 0xFF,0xFF,0x18,0x18,0x18,0x3C,0x24,0x00 };
const uint8_t PROGMEM ic_aleron_tras[8]  = { 0x24,0x3C,0x18,0x18,0x18,0xFF,0xFF,0x00 };
const uint8_t PROGMEM ic_pontones[8]  = { 0x18,0x7E,0x5A,0x3C,0x3C,0x5A,0x7E,0x18 };
const uint8_t PROGMEM ic_halo[8]   = { 0x38,0x44,0x7C,0x10,0x10,0x7C,0x44,0x38 };

struct Pieza { uint8_t x, y; uint8_t mascaraTipo; };
Pieza piezaActual;

uint8_t cocheX, cocheY;
int8_t dirX=1, dirY=0;
int8_t solX=1, solY=0;
unsigned long ultimoPasoF1=0;
uint16_t pasoMsF1=170;
bool pausaF1=false, parpadeoPausaF1=false;

unsigned long ultimaAnim=0;
uint8_t faseAnim=0;

bool f1ConColision = false;
uint8_t ticks_escudo_reaparicion = 0;

// efecto aire sucio
#if ACTIVAR_ESTELA
struct Estela { uint8_t x, y, vida; int8_t dx, dy; };
#define ESTELA_MAX 12
Estela bufferEstela[ESTELA_MAX];
uint8_t cabezaEstela = 0;
void anadirEstela(uint8_t x, uint8_t y, int8_t dx, int8_t dy){ bufferEstela[cabezaEstela] = {x,y,3,dx,dy}; if (++cabezaEstela >= ESTELA_MAX) cabezaEstela=0; }
void actualizarEstela(){
  for (uint8_t i=0;i<ESTELA_MAX;i++){
    if (bufferEstela[i].vida==0) continue;
    if ((faseAnim & 1) == 0) { int nx = (int)bufferEstela[i].x + (bufferEstela[i].dx); if (nx<0) nx = COLUMNAS-1; else if (nx>=COLUMNAS) nx=0; bufferEstela[i].x = (uint8_t)nx; }
    else { int ny = (int)bufferEstela[i].y + (bufferEstela[i].dy); if (ny<0) ny = FILAS-1; else if (ny>=FILAS) ny=0; bufferEstela[i].y = (uint8_t)ny; }
    if ((faseAnim & 3)==0 && bufferEstela[i].vida>0) bufferEstela[i].vida--;
  }
}
void dibujarEstelaPuff(const Estela &p){
  int px = p.x*CELDA, py = p.y*CELDA;
  if (p.vida==3){ display.drawPixel(px+3, py+1, SSD1306_WHITE); display.drawPixel(px+2, py+3, SSD1306_WHITE); }
  else if (p.vida==2){ display.drawPixel(px+1, py+3, SSD1306_WHITE); }
  else if (p.vida==1){ display.drawPixel(px+3, py+3, SSD1306_WHITE); }
}
void dibujarEstela(){ for (uint8_t i=0;i<ESTELA_MAX;i++) if (bufferEstela[i].vida>0) dibujarEstelaPuff(bufferEstela[i]); }
#else
inline void anadirEstela(uint8_t, uint8_t, int8_t, int8_t) {}
inline void actualizarEstela() {}
inline void dibujarEstela() {}
#endif

// obstaculos de curva s
enum DisenioF1 { DIS_S };
DisenioF1 disenioF1 = DIS_S;

bool esObstaculo(uint8_t x, uint8_t y){
  if (!f1ConColision) return false;
  if (y==0) return false;
  bool muroI = (x==5  && y>=1 && y<=6 && !(y==2 || y==3));
  bool muroD = (x==10 && y>=1 && y<=6 && !(y==4 || y==5));
  bool vertice  = ( (x==7 && y==1) || (x==8 && y==6) );
  return muroI || muroD || vertice;
}
void dibujarCelda(uint8_t gx, uint8_t gy){ int px=gx*CELDA, py=gy*CELDA; display.fillRect(px+1,py+1,CELDA-2,CELDA-2,SSD1306_WHITE); }
void dibujarObstaculos(){
  if (!f1ConColision) return;
  for (uint8_t y=1;y<=6;y++) if (!(y==2 || y==3)) dibujarCelda(5,y);
  for (uint8_t y=1;y<=6;y++) if (!(y==4 || y==5)) dibujarCelda(10,y);
  dibujarCelda(7,1); dibujarCelda(8,6);
}

// garaje secuencia
uint8_t secuenciaGaraje[6] = {PARTE_NARIZ, PARTE_RUEDAS, PARTE_ALERON_DEL, PARTE_ALERON_TRAS, PARTE_PONTONES, PARTE_HALO};
uint8_t indiceGaraje=0, mascaraGaraje=0; unsigned long ultimoGaraje=0;

// helpers
bool dentro(int16_t x, int16_t y){ return x>=0 && y>=0 && x<COLUMNAS && y<FILAS; }
bool caminoSeguro(uint8_t x, uint8_t y, int8_t dx, int8_t dy, uint8_t pasos){
  int16_t cx=x, cy=y;
  for (uint8_t i=0;i<pasos;i++){ cx += dx; cy += dy; if (!dentro(cx,cy)) return false; if (esObstaculo((uint8_t)cx,(uint8_t)cy)) return false; }
  return true;
}

void f1GenerarPieza() {
  uint8_t disp[6]; uint8_t n=0;
  if (!(piezasRecolectadas & PARTE_NARIZ))   disp[n++] = PARTE_NARIZ;
  if (!(piezasRecolectadas & PARTE_RUEDAS)) disp[n++] = PARTE_RUEDAS;
  if (!(piezasRecolectadas & PARTE_ALERON_DEL)) disp[n++] = PARTE_ALERON_DEL;
  if (!(piezasRecolectadas & PARTE_ALERON_TRAS))  disp[n++] = PARTE_ALERON_TRAS;
  if (!(piezasRecolectadas & PARTE_PONTONES))  disp[n++] = PARTE_PONTONES;
  if (!(piezasRecolectadas & PARTE_HALO))   disp[n++] = PARTE_HALO;
  if (n==0) return;

  piezaActual.mascaraTipo = disp[random(n)];
  uint16_t intentos=0;
  do {
    piezaActual.x = (uint8_t)random(COLUMNAS); 
    piezaActual.y = (uint8_t)random(1,FILAS); // no en la fila del HUD
    intentos++;
  } while (((piezaActual.x==cocheX && piezaActual.y==cocheY) || (f1ConColision && esObstaculo(piezaActual.x,piezaActual.y))) && intentos<400);

  if (intentos>=400){
    for (uint8_t yy=1; yy<FILAS; ++yy){
      for (uint8_t xx=0; xx<COLUMNAS; ++xx){
        if ((xx!=cocheX || yy!=cocheY) && (!f1ConColision || !esObstaculo(xx,yy))){ piezaActual.x = xx; piezaActual.y = yy; return; }
      }
    }
  }
}

void f1BuscarInicioSeguro() {
  for (uint16_t intentos=0; intentos<300; ++intentos) {
    uint8_t x = (uint8_t)random(COLUMNAS);
    uint8_t y = (uint8_t)random(1, FILAS);
    if (esObstaculo(x,y)) continue;
    int8_t dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    uint8_t start = random(4);
    for (uint8_t k=0; k<4; ++k) {
      int8_t dx = dirs[(start+k)&3][0];
      int8_t dy = dirs[(start+k)&3][1];
      if (!caminoSeguro(x,y,dx,dy,4)) continue;
      cocheX = x; cocheY = y; dirX = dx; dirY = dy; solX = dx; solY = dy;
      ticks_escudo_reaparicion = 6;
      return;
    }
  }
  cocheX = COLUMNAS/2; cocheY = FILAS/2; dirX = -1; dirY = 0; solX = -1; solY = 0; ticks_escudo_reaparicion = 6;
}

void f1ReiniciarJuego() {
  piezasRecolectadas = 0;
  pasoMsF1 = 170;
  pausaF1 = false; parpadeoPausaF1=false;
  faseAnim = 0; ultimaAnim = millis();
#if ACTIVAR_ESTELA
  for (uint8_t i=0;i<ESTELA_MAX;i++) bufferEstela[i].vida = 0; cabezaEstela = 0;
#endif
  if (f1ConColision) f1BuscarInicioSeguro();
  else { cocheX = COLUMNAS/2; cocheY = FILAS/2; dirX = 1; dirY = 0; solX = 1; solY = 0; ticks_escudo_reaparicion = 0; }
  f1GenerarPieza();
}

// dibujo ruedas
void dibujarRuedasUD(int px, int py, bool arriba, uint8_t piezas){
  if (!(piezas & PARTE_RUEDAS)) return;
  display.fillRect(px+0, py+(arriba?1:5), 2, 2, SSD1306_WHITE);
  display.fillRect(px+6, py+(arriba?1:5), 2, 2, SSD1306_WHITE);
  display.fillRect(px+0, py+(arriba?5:1), 2, 2, SSD1306_WHITE);
  display.fillRect(px+6, py+(arriba?5:1), 2, 2, SSD1306_WHITE);
}
void dibujarRuedasLR(int px, int py, bool izquierda, uint8_t piezas){
  if (!(piezas & PARTE_RUEDAS)) return;
  display.fillRect(px+(izquierda?1:5), py+0, 2, 2, SSD1306_WHITE);
  display.fillRect(px+(izquierda?1:5), py+6, 2, 2, SSD1306_WHITE);
  display.fillRect(px+(izquierda?5:1), py+0, 2, 2, SSD1306_WHITE);
  display.fillRect(px+(izquierda?5:1), py+6, 2, 2, SSD1306_WHITE);
}

// dibujo coche 8x8
void f1DibujarCeldaCoche(int cx, int cy, uint8_t piezas) {
  int px = cx*CELDA, py = cy*CELDA;

  if (dirY < 0) {
    display.drawFastVLine(px+3, py+2, 5, SSD1306_WHITE);
    display.drawFastVLine(px+4, py+2, 5, SSD1306_WHITE);
    display.drawPixel(px+2, py+2, SSD1306_WHITE);
    display.drawPixel(px+5, py+2, SSD1306_WHITE);
    display.fillRect(px+3, py+3, 2, 2, SSD1306_WHITE);
    if (piezas & PARTE_PONTONES){ display.drawRect(px+1, py+3, 2, 2, SSD1306_WHITE); display.drawRect(px+5, py+3, 2, 2, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_DEL){ display.drawFastHLine(px+0, py+0, 8, SSD1306_WHITE); display.drawFastHLine(px+1, py+1, 6, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_TRAS){  display.drawFastHLine(px+2, py+7, 4, SSD1306_WHITE); display.drawFastHLine(px+3, py+6, 2, SSD1306_WHITE); }
    if (piezas & PARTE_NARIZ){   display.drawLine(px+4, py+2, px+4, py+0, SSD1306_WHITE); }
    if (piezas & PARTE_HALO){   display.drawFastHLine(px+2, py+2, 4, SSD1306_WHITE); display.drawFastVLine(px+4, py+2, 2, SSD1306_WHITE); }
    dibujarRuedasUD(px, py, true, piezas);

  } else if (dirY > 0) {
    display.drawFastVLine(px+3, py+1, 5, SSD1306_WHITE);
    display.drawFastVLine(px+4, py+1, 5, SSD1306_WHITE);
    display.drawPixel(px+2, py+5, SSD1306_WHITE);
    display.drawPixel(px+5, py+5, SSD1306_WHITE);
    display.fillRect(px+3, py+3, 2, 2, SSD1306_WHITE);
    if (piezas & PARTE_PONTONES){ display.drawRect(px+1, py+3, 2, 2, SSD1306_WHITE); display.drawRect(px+5, py+3, 2, 2, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_DEL){ display.drawFastHLine(px+0, py+7, 8, SSD1306_WHITE); display.drawFastHLine(px+1, py+6, 6, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_TRAS){  display.drawFastHLine(px+2, py+0, 4, SSD1306_WHITE); display.drawFastHLine(px+3, py+1, 2, SSD1306_WHITE); }
    if (piezas & PARTE_NARIZ){   display.drawLine(px+4, py+5, px+4, py+7, SSD1306_WHITE); }
    if (piezas & PARTE_HALO){   display.drawFastHLine(px+2, py+4, 4, SSD1306_WHITE); display.drawFastVLine(px+4, py+3, 2, SSD1306_WHITE); }
    dibujarRuedasUD(px, py, false, piezas);

  } else if (dirX < 0) {
    display.drawFastHLine(px+1, py+3, 5, SSD1306_WHITE);
    display.drawFastHLine(px+1, py+4, 5, SSD1306_WHITE);
    display.drawPixel(px+5, py+2, SSD1306_WHITE);
    display.drawPixel(px+5, py+5, SSD1306_WHITE);
    display.fillRect(px+3, py+3, 2, 2, SSD1306_WHITE);
    if (piezas & PARTE_PONTONES){ display.drawRect(px+3, py+1, 2, 2, SSD1306_WHITE); display.drawRect(px+3, py+5, 2, 2, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_DEL){ display.drawFastVLine(px+0, py+0, 8, SSD1306_WHITE); display.drawFastVLine(px+1, py+1, 6, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_TRAS){  display.drawFastVLine(px+7, py+2, 4, SSD1306_WHITE); display.drawFastVLine(px+6, py+3, 2, SSD1306_WHITE); }
    if (piezas & PARTE_NARIZ){   display.drawLine(px+1, py+4, px+0, py+4, SSD1306_WHITE); }
    if (piezas & PARTE_HALO){   display.drawFastVLine(px+4, py+2, 4, SSD1306_WHITE); display.drawFastHLine(px+3, py+4, 2, SSD1306_WHITE); }
    dibujarRuedasLR(px, py, true, piezas);

  } else {
    display.drawFastHLine(px+2, py+3, 5, SSD1306_WHITE);
    display.drawFastHLine(px+2, py+4, 5, SSD1306_WHITE);
    display.drawPixel(px+2, py+2, SSD1306_WHITE);
    display.drawPixel(px+2, py+5, SSD1306_WHITE);
    display.fillRect(px+3, py+3, 2, 2, SSD1306_WHITE);
    if (piezas & PARTE_PONTONES){ display.drawRect(px+3, py+1, 2, 2, SSD1306_WHITE); display.drawRect(px+3, py+5, 2, 2, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_DEL){ display.drawFastVLine(px+7, py+0, 8, SSD1306_WHITE); display.drawFastVLine(px+6, py+1, 6, SSD1306_WHITE); }
    if (piezas & PARTE_ALERON_TRAS){  display.drawFastVLine(px+0, py+2, 4, SSD1306_WHITE); display.drawFastVLine(px+1, py+3, 2, SSD1306_WHITE); }
    if (piezas & PARTE_NARIZ){   display.drawLine(px+6, py+4, px+7, py+4, SSD1306_WHITE); }
    if (piezas & PARTE_HALO){   display.drawFastVLine(px+4, py+2, 4, SSD1306_WHITE); display.drawFastHLine(px+4, py+4, 2, SSD1306_WHITE); }
    dibujarRuedasLR(px, py, false, piezas);
  }
}

void trazarRayadoRect(int X,int Y,int W,int H,int paso){ for (int yy=Y; yy<Y+H; yy+=paso) display.drawLine(X, yy, X+W-1, yy, SSD1306_WHITE); }

//coche garaje
void dibujarCocheGrande(uint8_t cx, uint8_t cy, uint8_t piezas) {
  int x = cx*CELDA - 8, y = cy*CELDA - 8;
  display.drawRect(x+3, y+3, 10, 10, SSD1306_WHITE);
  trazarRayadoRect(x+3, y+3, 10, 10, 2);
  display.fillRect(x+6, y+6, 4, 4, SSD1306_WHITE);
  if (piezas & PARTE_NARIZ)   { trazarRayadoRect(x+6, y, 4, 3, 3); display.drawLine(x+7, y+2, x+8, y+0, SSD1306_WHITE); display.drawPixel(x+8, y+1, SSD1306_WHITE); }
  if (piezas & PARTE_ALERON_DEL) { trazarRayadoRect(x+1, y, 12, 2, 3); display.drawLine(x+1, y+1, x+14, y+1, SSD1306_WHITE); }
  if (piezas & PARTE_PONTONES)  { display.drawRect(x+1,  y+7, 3, 2, SSD1306_WHITE); display.drawRect(x+12, y+7, 3, 2, SSD1306_WHITE); }
  if (piezas & PARTE_RUEDAS) { display.fillRect(x+0,  y+3, 2, 3, SSD1306_WHITE); display.fillRect(x+0,  y+10,2, 3, SSD1306_WHITE);
                             display.fillRect(x+14, y+3, 2, 3, SSD1306_WHITE); display.fillRect(x+14, y+10,2, 3, SSD1306_WHITE); }
  if (piezas & PARTE_ALERON_TRAS)  { trazarRayadoRect(x+4, y+15, 8, 1, 3); display.drawLine(x+4, y+15, x+11, y+15, SSD1306_WHITE); }
  if (piezas & PARTE_HALO)   { display.drawLine(x+5, y+4, x+10, y+4, SSD1306_WHITE); display.drawLine(x+7, y+4, x+7,  y+7, SSD1306_WHITE); }
}

//piezas a recoger
void f1DibujarPieza() {
  int parpadeo = (faseAnim & 0x08) ? 1 : 0;
  int px = piezaActual.x*CELDA, py = piezaActual.y*CELDA + parpadeo;
  const uint8_t* bmp = ic_nariz;
  switch (piezaActual.mascaraTipo) {
    case PARTE_NARIZ: bmp = ic_nariz; break; case PARTE_RUEDAS: bmp = ic_ruedas; break;
    case PARTE_ALERON_DEL: bmp = ic_aleron_del; break; case PARTE_ALERON_TRAS: bmp = ic_aleron_tras; break;
    case PARTE_PONTONES: bmp = ic_pontones; break; case PARTE_HALO: bmp = ic_halo; break;
  }
  bool parp = (faseAnim & 0x10);
  display.drawBitmap(px, py, bmp, 8, 8, SSD1306_WHITE);
  if (parp) display.drawRect(px, py, 8, 8, SSD1306_WHITE);
}

// HUD garaje F1
void f1DibujarHUD() {
  int x = 0;
  const uint8_t* iconos[6] = {ic_nariz, ic_ruedas, ic_aleron_del, ic_aleron_tras, ic_pontones, ic_halo};
  uint8_t mascaras[6] = {PARTE_NARIZ, PARTE_RUEDAS, PARTE_ALERON_DEL, PARTE_ALERON_TRAS, PARTE_PONTONES, PARTE_HALO};
  for (int i=0;i<6;i++) {
    display.drawBitmap(x, 0, iconos[i], 8, 8, SSD1306_WHITE);
    if (piezasRecolectadas & mascaras[i]) display.drawRect(x,0,8,8,SSD1306_WHITE);
    x += 10;
  }
  char buf[20];
  snprintf(buf, sizeof(buf), "GAR: %u", (unsigned)conteo_garaje);
  int16_t w = (int16_t)strlen(buf) * 6;
  int16_t xText = ANCHO_PANTALLA - w - 2;
  if (xText < x + 2) xText = x + 2;
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(xText, 0);
  display.print(buf);
}

// dibujo partida f1
void f1DibujarJuego(bool overlayPausa) {
  if (!oled_ok) return;
  display.clearDisplay();
  f1DibujarHUD();
  dibujarObstaculos();
  if (piezasRecolectadas != PARTE_TODO) f1DibujarPieza();
  dibujarEstela();
  f1DibujarCeldaCoche(cocheX, cocheY, piezasRecolectadas);
  if (overlayPausa) {
    if (parpadeoPausaF1) display.fillRect(18, 22, 92, 20, SSD1306_WHITE);
    display.drawRect(18, 22, 92, 20, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(parpadeoPausaF1 ? SSD1306_BLACK : SSD1306_WHITE);
    display.setCursor(42, 28); display.print(F("PAUSA"));
  }
  display.display();
}

//tenis
int jugY, cpuY;
const int padAncho=2, padAlto=12;
const int jugX=3, cpuX=ANCHO_PANTALLA-3-padAncho;
int bolaX, bolaY, vX, vY;
int puntuacionJ=0, puntuacionCPU=0;
bool pausaTenis=false;
uint8_t modoTenis=1; // 0 facil, 1 normal, 2 imposible
unsigned long ultimoPasoTenis=0;
uint8_t pasoMsTenis = 16;

int signo(int v){ return (v<0)?-1:1; }

void aleatorizarEnGolpe(int offsetCentro, uint8_t modo, int signoVXDeseado){
  int sxMin[3] = {1, 1, 2};
  int sxMax[3] = {4, 4, 5};
  int magX = random(sxMin[modo], sxMax[modo]+1);
  vX = signoVXDeseado * magX;

  int base = (offsetCentro>0)? 1 : -1;
  int sesgo = min(abs(offsetCentro)/2, (modo==2)?2:3);
  int dispersion[3] = {5, 3, 2};
  int ry = random(-dispersion[modo], dispersion[modo]+1);
  int probEfecto[3] = {40, 25, 18};
  int efecto = (random(100) < probEfecto[modo]) ? (random(0,2)?1:-1) : 0;

  vY = base * (1 + sesgo) + ry + efecto;

  int probVoltear[3] = {15, 10, 4};
  if (random(100) < probVoltear[modo]) vY = -vY;

  if (vY==0) vY = (random(0,2)? 1 : -1);
  int lim = 3;
  if (vY>lim) vY=lim; if (vY<-lim) vY=-lim;
}

void empujonPared(uint8_t modo){
  int prob[3] = {50, 40, 30};
  if (random(100) < prob[modo]) {
    int delta = (random(0,2)? 1 : -1);
    int nv = vX + delta; if (nv==0) nv = delta;
    int tope = (modo==2)?5:4;
    if (nv>tope) nv=tope; if (nv<-tope) nv=-tope;
    vX = nv;
  }
}

void acelerarBola(uint8_t modo, bool enPala){
  if (modo!=2) return;
  int maxVX = 5, maxVY = 3;
  if (abs(vX) < maxVX && random(0,3)==0) vX += signo(vX);
  if (abs(vY) < maxVY && (enPala ? random(0,2)==0 : random(0,4)==0)) vY += signo(vY);
}

void tenisReiniciarPartido(uint8_t modo){
  modoTenis = modo;
  puntuacionJ = puntuacionCPU = 0;
  jugY = cpuY = (ALTO_PANTALLA - padAlto)/2;
  bolaX = ANCHO_PANTALLA/2; bolaY = ALTO_PANTALLA/2;
  vX = ((modo==2)? 3 : 2) * (random(0,2)? 1 : -1);
  vY = (random(0,2)? 1 : -1);
  pausaTenis = false;
}
void tenisSaque(bool haciaCPU=true){
  bolaX = ANCHO_PANTALLA/2; bolaY = ALTO_PANTALLA/2;
  vX = ((modoTenis==2)? 3 : 2) * (haciaCPU ? 1 : -1);
  vY = (random(0,2)? 1 : -1);
  sfxSaque();
}
void tenisLeerEntrada(){
  if (digitalRead(PIN_ARRIBA)==LOW)   { jugY -= 3; if (jugY<0) jugY=0; }
  if (digitalRead(PIN_ABAJO)==LOW) { jugY += 3; if (jugY>ALTO_PANTALLA-padAlto) jugY=ALTO_PANTALLA-padAlto; }

  if (actualizarBoton(bOK)   && bOK.estable)   { pausaTenis = !pausaTenis; sfxPausa(pausaTenis); }
  if (actualizarBoton(bMenu) && bMenu.estable) { sfxSeleccionarMenu(); estado = EST_TNS_MENU; }
}

void tenisIA(){
  const int velBase [3] = {2, 2, 3};
  const int jitterMax[3]  = {8, 4, 1};
  const uint8_t errPct[3] = {40,18,6};
  const uint8_t congelMax[3]={12,6,2};
  const uint8_t pensarCada[3]={4,2,1};
  const uint8_t malLecturaPct[3]={35,15,3};
  const uint8_t anticipa[3] ={40,70,100};

  static uint8_t congelado = 0;
  static uint8_t divisorPensar = 0;

  if (congelado>0){ congelado--; return; }

  if ((divisorPensar++ % pensarCada[modoTenis]) != 0) {
    if (vX < 0) {
      int mid = (ALTO_PANTALLA - padAlto)/2 + random(-2,3);
      if (cpuY + padAlto/2 < mid) cpuY += 1;
      else if (cpuY + padAlto/2 > mid) cpuY -= 1;
    }
    return;
  }

  int dx = (cpuX - (bolaX+3));
  int vx = (vX==0) ? 1 : vX;
  int ticks = dx / (abs(vx)); if (ticks<0) ticks = 0;
  ticks = (ticks * anticipa[modoTenis]) / 100;

  int simY = bolaY;
  int simVY = vY == 0 ? 1 : vY;
  int tmpTicks = ticks;
  while (tmpTicks-- > 0){
    simY += simVY;
    if (simY <= 2)               { simY = 2;               simVY = -simVY; }
    else if (simY >= ALTO_PANTALLA-5) { simY = ALTO_PANTALLA-5; simVY = -simVY; }
  }
  int predY = simY;

  if (random(100) < malLecturaPct[modoTenis]) {
    predY += random(-18, 19);
    if (random(2)==0) predY = ALTO_PANTALLA - predY;
  }

  predY += (int)random(-jitterMax[modoTenis], jitterMax[modoTenis]+1);

  if (random(100) < errPct[modoTenis]) {
    if (random(2)==0) { uint8_t congelMaxVal = congelMax[modoTenis]; if (congelMaxVal==0) congelMaxVal=1; congelado = (uint8_t)random(1, congelMaxVal+1); }
    else { predY += (int)random(-14, 15); }
  }

  int centro = cpuY + padAlto/2;
  if (centro < predY-1) cpuY += velBase[modoTenis];
  else if (centro > predY+1) cpuY -= velBase[modoTenis];

  if (cpuY<0) cpuY=0; if (cpuY>ALTO_PANTALLA-padAlto) cpuY=ALTO_PANTALLA-padAlto;
}

void tenisAvanzarPaso(){
  if (pausaTenis) return;
  bolaX += vX; 
  bolaY += vY;

  if (bolaY <= 2) { bolaY = 2; vY = -vY; sfxPared(); empujonPared(modoTenis); acelerarBola(modoTenis,false); }
  if (bolaY >= ALTO_PANTALLA-5) { bolaY = ALTO_PANTALLA-5; vY = -vY; sfxPared(); empujonPared(modoTenis); acelerarBola(modoTenis,false); }

  if (bolaX <= jugX+padAncho && bolaX >= jugX && bolaY+3 >= jugY && bolaY <= jugY+padAlto){
    bolaX = jugX+padAncho;
    int offset = (bolaY + 1) - (jugY + padAlto/2);
    aleatorizarEnGolpe(offset, modoTenis, +1);
    sfxPong();
    acelerarBola(modoTenis,true);
  }
  if (bolaX+3 >= cpuX && bolaX <= cpuX+padAncho && bolaY+3 >= cpuY && bolaY <= cpuY+padAlto){
    bolaX = cpuX-3;
    int offset = (bolaY + 1) - (cpuY + padAlto/2);
    aleatorizarEnGolpe(offset, modoTenis, -1);
    sfxPong();
    acelerarBola(modoTenis,true);
  }

  if (bolaX < 0){ puntuacionCPU++; sfxPuntuacion(); estado = EST_TNS_PUNTO; pausaTenis=true; }
  else if (bolaX > ANCHO_PANTALLA){ puntuacionJ++; sfxPuntuacion(); estado = EST_TNS_PUNTO; pausaTenis=true; }

  tenisIA();
}

//dibujo campo tenis
void dibujarSoloPista(){
  if (!oled_ok) return;
  display.drawRect(2, 2, ANCHO_PANTALLA-4, ALTO_PANTALLA-4, SSD1306_WHITE);
  for (int y=4; y<ALTO_PANTALLA-4; y+=6) display.drawFastVLine(ANCHO_PANTALLA/2, y, 3, SSD1306_WHITE);
  display.drawFastVLine(ANCHO_PANTALLA/4, 8, ALTO_PANTALLA-16, SSD1306_WHITE);
  display.drawFastVLine(3*ANCHO_PANTALLA/4, 8, ALTO_PANTALLA-16, SSD1306_WHITE);
}

// superposicion HUD
void tenisDibujar(){
  if (!oled_ok) return;
  display.clearDisplay();
  dibujarSoloPista();
  display.fillRect(jugX, jugY, padAncho, padAlto, SSD1306_WHITE);
  display.fillRect(cpuX, cpuY, padAncho, padAlto, SSD1306_WHITE);
  display.fillRect(bolaX, bolaY, 3, 3, SSD1306_WHITE);

  char marcador[8];
  snprintf(marcador, sizeof(marcador), "%d-%d", puntuacionJ, puntuacionCPU);
  dibujarCajaTextoCentrada(marcador, 0, 1, 1);

  const char* txtModo = (modoTenis==0)? "FAC" : (modoTenis==1)? "NOR" : "IMP";
  dibujarCajaTexto(2, 0, txtModo, 1, 1);

  if (pausaTenis) dibujarCajaTextoCentrada("PAUSA", 56, 1, 1);

  display.display();
}

//menu principal
void dibujarMenuPrincipal(){
  if (!oled_ok) return;
  display.clearDisplay();
  dibujarTitulo(F("JUEGOS"), 6, 6);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  int y0=28;
  display.setCursor(20,y0);      if(idxMenuPrincipal==0) display.print(F("> F1 Botey"));  else display.print(F("  F1 Botey"));
  display.setCursor(20,y0+12);   if(idxMenuPrincipal==1) display.print(F("> Tenis"));     else display.print(F("  Tenis"));
  display.setCursor(20,y0+24);   if(idxMenuPrincipal==2) display.print(F("> Creditos"));  else display.print(F("  Creditos"));
  display.display();
}
void leerMenuPrincipal(){
  if (actualizarBoton(bArriba)   && bArriba.estable)   { if(idxMenuPrincipal>0) idxMenuPrincipal--; sfxMoverMenu(); }
  if (actualizarBoton(bAbajo) && bAbajo.estable) { if(idxMenuPrincipal<2) idxMenuPrincipal++; sfxMoverMenu(); }
  bool seleccionar = (actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable);
  if (seleccionar){
    sfxSeleccionarMenu();
    if (idxMenuPrincipal==0) estado = EST_F1_MENU;
    else if (idxMenuPrincipal==1) estado = EST_TNS_MENU;
    else { yCreditos = ALTO_PANTALLA; ultCreditos = millis(); estado = EST_CREDITOS; }
  }
}

//menu F1
void dibujarMenuF1(){
  if (!oled_ok) return;
  display.clearDisplay();
  dibujarTitulo(F("F1 BOTEY"), 8, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  const int y0 = 20, line = 10;
  display.setCursor(6, y0 + 0*line); if (idxMenuF1==0) display.print(F("> Sin colision")); else display.print(F("  Sin colision"));
  display.setCursor(6, y0 + 1*line); if (idxMenuF1==1) display.print(F("> Colision: Curva S"));  else display.print(F("  Colision: Curva S"));
  display.setCursor(6, y0 + 2*line); if (idxMenuF1==2) display.print(F("> Menu principal"));     else display.print(F("  Menu principal"));
  display.display();
}
void leerMenuF1(){
  if (actualizarBoton(bArriba)   && bArriba.estable)   { if(idxMenuF1>0) idxMenuF1--; sfxMoverMenu(); }
  if (actualizarBoton(bAbajo) && bAbajo.estable) { if(idxMenuF1<2) idxMenuF1++; sfxMoverMenu(); }
  bool seleccionar = (actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable);
  if (seleccionar){
    sfxSeleccionarMenu();
    if (idxMenuF1==0){ f1ConColision=false; f1ReiniciarJuego(); estado = EST_F1_JUEGO; }
    else if (idxMenuF1==1){ f1ConColision=true; f1ReiniciarJuego(); estado = EST_F1_JUEGO; }
    else { estado = EST_SELEC_JUEGO; }
  }
}

//menu tenis
void dibujarMenuTenis(){
  if (!oled_ok) return;
  display.clearDisplay();
  dibujarTitulo(F("TENIS"), 5, 6);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  int y0=26;
  display.setCursor(16,y0);      if(idxMenuTenis==0) display.print(F("> Facil"));      else display.print(F("  Facil"));
  display.setCursor(16,y0+10);   if(idxMenuTenis==1) display.print(F("> Normal"));     else display.print(F("  Normal"));
  display.setCursor(16,y0+20);   if(idxMenuTenis==2) display.print(F("> Imposible"));  else display.print(F("  Imposible"));
  display.setCursor(16,y0+30);   if(idxMenuTenis==3) display.print(F("> Menu principal")); else display.print(F("  Menu principal"));
  display.display();
}
void leerMenuTenis(){
  if (actualizarBoton(bArriba)   && bArriba.estable)   { if(idxMenuTenis>0) idxMenuTenis--; sfxMoverMenu(); }
  if (actualizarBoton(bAbajo) && bAbajo.estable) { if(idxMenuTenis<3) idxMenuTenis++; sfxMoverMenu(); }
  bool seleccionar = (actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable);
  if (seleccionar){
    sfxSeleccionarMenu();
    if (idxMenuTenis<=2){ tenisReiniciarPartido(idxMenuTenis); estado = EST_TNS_JUEGO; }
    else { estado = EST_SELEC_JUEGO; }
  }
}

// creditos
void dibujarCreditos(){
  if (!oled_ok) return;
  if (millis() - ultCreditos >= 70) { ultCreditos = millis(); yCreditos--; }
  display.clearDisplay();
  int y = yCreditos;
  dibujarCentradoP(F("GRACIAS A"), y, 2);  y += 28;
  dibujarCentradoP(F("Yo (JP)"),     y, 1); y += 12;
  dibujarCentradoP(F("Luis"),        y, 1); y += 12;
  dibujarCentradoP(F("Jaume Botey"), y, 1); y += 12;
  dibujarCentradoP(F("Pere"),        y, 1); y += 12;
  dibujarCentradoP(F("Laia"),        y, 1); y += 16;
  dibujarCentradoP(F("Por ultimo a todos"), y, 1); y += 12;
  dibujarCentradoP(F("los que ayudaron"),   y, 1); y += 12;
  dibujarCentradoP(F("durante la"),         y, 1); y += 12;
  dibujarCentradoP(F("programacion"),       y, 1); y += 12;
  display.display();
  if (y < 0) yCreditos = ALTO_PANTALLA + 6;
}
void leerCreditos(){
  if ((actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable)) { sfxSeleccionarMenu(); estado = EST_SELEC_JUEGO; }
}

// setup
void setup() {
  pinMode(PIN_ZUMBADOR, OUTPUT);
  Wire.begin();
  Wire.setClock(400000); // configuracion para mas fluidez (min 100000)

  uint8_t addrs[2] = {0x3C, 0x3D};
  for (uint8_t i=0; i<2; i++){
    if (display.begin(SSD1306_SWITCHCAPVCC, addrs[i])) { oled_ok = true; break; }
    delay(40);
  }
  if (oled_ok){
    display.clearDisplay(); display.display();
    display.setTextWrap(false); // evita que el contador “salte” de línea
  } else {
    for (int i=0;i<3;i++){ tone(PIN_ZUMBADOR, 450, 150); delay(200); }
  }

  pinMode(PIN_IZQ,  INPUT_PULLUP);
  pinMode(PIN_DER, INPUT_PULLUP);
  pinMode(PIN_ABAJO,  INPUT_PULLUP);
  pinMode(PIN_ARRIBA,    INPUT_PULLUP);
  pinMode(PIN_OK,    INPUT_PULLUP);
  pinMode(PIN_MENU,  INPUT_PULLUP);

  randomSeed(analogRead(A0)); // cambiar las posiciones aleatorimente
  cargarGaraje();
  estado = EST_SELEC_JUEGO;
}

// controles F1
void f1LeerEntrada() {
  if (digitalRead(PIN_ARRIBA)==LOW)   { solX =  0; solY = -1; }
  if (digitalRead(PIN_ABAJO)==LOW) { solX =  0; solY =  1; }
  if (digitalRead(PIN_IZQ)==LOW) { solX = -1; solY =  0; }
  if (digitalRead(PIN_DER)==LOW){ solX =  1; solY =  0; }
  if (solX == -dirX && solY == -dirY) { solX = dirX; solY = dirY; }

  if (actualizarBoton(bOK) && bOK.estable)    { pausaF1 = !pausaF1; parpadeoPausaF1 = true; sfxPausa(pausaF1); }
  if (actualizarBoton(bMenu) && bMenu.estable){ sfxSeleccionarMenu(); estado = EST_F1_MENU; }
}

// movimiento, choques y recogida
void f1AvanzarPaso() {
  if (!(solX == -dirX y solY == -dirY)) { dirX = solX; dirY = solY; }
  int16_t nx = (int16_t)cocheX + dirX, ny = (int16_t)cocheY + dirY;

  if (f1ConColision y ticks_escudo_reaparicion==0) {
    if (nx < 0 || ny < 0 || nx >= COLUMNAS || ny >= FILAS) { sfxChoque(); estado = EST_F1_FIN; return; }
  } else if (!f1ConColision) {
    if (nx < 0) nx = COLUMNAS-1; else if (nx >= COLUMNAS) nx = 0; // sale por la izquierda -> entra por la derecha y lo mismo viceversa
    if (ny < 0) ny = FILAS-1; else if (ny >= FILAS) ny = 0; // sale por arriba -> entra por abajo y lo mismo viceversa
  }

  if (f1ConColision & ticks_escudo_reaparicion==0 y esObstaculo((uint8_t)nx,(uint8_t)ny)) { sfxChoque(); estado = EST_F1_FIN; return; }

  anadirEstela(cocheX, cocheY, dirX, dirY);
  cocheX = (uint8_t)nx; cocheY = (uint8_t)ny;
  if (ticks_escudo_reaparicion>0) ticks_escudo_reaparicion--;
  if ((faseAnim & 1)==0) actualizarEstela();

  if ((piezasRecolectadas != PARTE_TODO) y cocheX == piezaActual.x && cocheY == piezaActual.y) {
    piezasRecolectadas |= piezaActual.mascaraTipo; sfxRecoger();
    if (piezasRecolectadas == PARTE_TODO) {
      indiceGaraje = 0; mascaraGaraje = 0; ultimoGaraje = millis();
      estado = EST_F1_GARAJE; return;
    } else {
      f1GenerarPieza();
    }
  }
}

// loop principal
void loop() {
  static unsigned long ultimaAnimLocal = 0;
  unsigned long ahora = millis();
  if (ahora - ultimaAnimLocal >= 90) { ultimaAnimLocal = ahora; faseAnim++; if (pausaF1) parpadeoPausaF1 = !parpadeoPausaF1; }

  switch (estado) {
    case EST_SELEC_JUEGO:   leerMenuPrincipal(); dibujarMenuPrincipal(); break;

    case EST_F1_MENU:   leerMenuF1(); dibujarMenuF1(); break;

    case EST_F1_JUEGO: {
      f1LeerEntrada();
      if (!pausaF1 && (ahora - ultimoPasoF1 >= pasoMsF1)) { ultimoPasoF1 = ahora; f1AvanzarPaso(); }
      f1DibujarJuego(pausaF1);
    } break;

    case EST_F1_GARAJE: {
      if (indiceGaraje < 6 && (ahora - ultimoGaraje >= 320)) {
        mascaraGaraje |= secuenciaGaraje[indiceGaraje++]; ultimoGaraje = ahora; sfxPasoEnsamblaje();
        if (indiceGaraje==6) { conteo_garaje++; guardarGaraje(); }
      }
      if ((actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable)) {
        sfxSeleccionarMenu();
        piezasRecolectadas = 0;
        if (pasoMsF1 > 80) pasoMsF1 -= 8;
        f1GenerarPieza();
        estado = EST_F1_JUEGO;
      }
      if (!oled_ok) break;
      display.clearDisplay();
      display.drawRect(2, 10, 124, 50, SSD1306_WHITE);
      for (int yy=14; yy<=58; yy+=8) display.drawLine(4, yy, 124, yy, SSD1306_WHITE);
      { uint8_t cx = COLUMNAS/2, cy = FILAS/2; dibujarCocheGrande(cx, cy, mascaraGaraje); }
      display.fillRect(0, 0, 128, 16, SSD1306_BLACK);
      display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
      display.setCursor(4, 0);  display.print(F("GARAJE"));
      { char line[24]; snprintf(line, sizeof(line), "Coches: %u", (unsigned)conteo_garaje);
        int16_t w = (int16_t)strlen(line) * 6; int16_t x = ANCHO_PANTALLA - w - 2;
        display.setCursor(x, 0); display.print(line);
      }
      display.setCursor(4, 8); if (indiceGaraje<6) display.print(F("Ensamblando...")); else display.print(F("OK/Izquierda: seguir"));
      display.display();
    } break;

    case EST_F1_FIN: {
      if (oled_ok){
        display.clearDisplay();
        display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
        display.setCursor(10, 16); display.print(F("CHOQUE!"));
        display.setTextSize(1);
        display.setCursor(6, 42); display.print(F("OK/Izquierda: menu F1"));
        display.display();
      }
      if ((actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable)) { sfxSeleccionarMenu(); estado = EST_F1_MENU; }
    } break;

    case EST_TNS_MENU:  leerMenuTenis(); dibujarMenuTenis(); break;

    case EST_TNS_JUEGO: {
      tenisLeerEntrada();
      if (ahora - ultimoPasoTenis >= pasoMsTenis){ ultimoPasoTenis = ahora; tenisAvanzarPaso(); }
      tenisDibujar();
      if (puntuacionJ>=5 || puntuacionCPU>=5){ estado = EST_TNS_FIN; }
      if (estado==EST_TNS_PUNTO){
        bool haciaCPU = (puntuacionJ <= puntuacionCPU);
        tenisSaque(haciaCPU);
        estado = EST_TNS_JUEGO;
      }
    } break;

    case EST_TNS_FIN: {
      if (oled_ok){
        display.clearDisplay();
        display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
        display.setCursor(18, 16); display.print( (puntuacionJ>puntuacionCPU)? F("GANASTE") : F("PERDISTE") );
        display.setTextSize(1);
        display.setCursor(10, 44); display.print(F("OK/Izquierda: menu Tenis"));
        display.display();
      }
      if ((actualizarBoton(bDer)&&bDer.estable) || (actualizarBoton(bOK)&&bOK.estable)) { sfxSeleccionarMenu(); estado = EST_TNS_MENU; }
    } break;

    case EST_CREDITOS: leerCreditos(); dibujarCreditos(); break;
  }
}
