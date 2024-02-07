/* -*- C++ -*- */
/********************
Sept. 2014 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com

quadrature encoder driver (PCINT)
quadrature encoder stream (fake, not using buffers)

*/

#ifndef RSITE_ARDUINO_MENU_ENCODER
  #define RSITE_ARDUINO_MENU_ENCODER

  #include <pcint.h> //https://github.com/neu-rah/PCINT
  #include "../menuDefs.h"

  namespace Menu {

#define DEBOUNCE_MIN_US 500

    template<uint8_t pinA,uint8_t pinB>
    class encoderIn {
    public:
      volatile int pos=0;
      volatile int lastClockTime=0;
      volatile long debouncedClockPeriodUS=0;
      int8_t pos_adder = 1;  // This is 1 or -1 depending on direction/reversal
      //int pinA,pinB;
      //encoderIn<pinA,pinB>(int a,int b):pinA(a),pinB(b) {}
      void reverse() { pos_adder = -1 * pos_adder; }
      void begin() {
        pinMode(pinA, INPUT_PULLUP);
        pinMode(pinB, INPUT_PULLUP);
        //attach pin change handlers
        PCattachInterrupt<pinA>(mixHandler((void(*)(void*))encoderInUpdateA,this), CHANGE);
//        PCattachInterrupt<pinB>(mixHandler((void(*)(void*))encoderInUpdateB,this), CHANGE);
      }
      //PCint handlers
      static void encoderInUpdateA(class encoderIn<pinA,pinB> *e);
//      static void encoderInUpdateB(class encoderIn<pinA,pinB> *e);
    };

    //PCint handlers
    template<uint8_t pinA,uint8_t pinB>
    void encoderIn<pinA,pinB>::encoderInUpdateA(class encoderIn<pinA,pinB> *e) {
      int a = digitalRead(pinA);
      int b = digitalRead(pinB);
      long start = micros();
      long nobounce_us = start - e->lastClockTime;
      if (nobounce_us >= DEBOUNCE_MIN_US) {
        e->lastClockTime = start;
        e->debouncedClockPeriodUS = nobounce_us;
        if (a == b)
          e->pos += e->pos_adder;
        else
          e->pos -= e->pos_adder;
      }
    }
    // template<uint8_t pinA,uint8_t pinB>
    // void encoderIn<pinA,pinB>::encoderInUpdateB(class encoderIn<pinA,pinB> *e) {
    //   if (digitalRead(pinA)^digitalRead(pinB)) e->pos++;
    //   else e->pos--;
    // }

    //emulate a stream based on encoderIn movement returning +/- for every 'sensivity' steps
    //buffer not needer because we have an accumulator
    template<uint8_t pinA,uint8_t pinB>
    class encoderInStream:public menuIn {
    public:
      encoderIn<pinA,pinB> &enc;//associated hardware encoderIn
      int sensivity;
      int oldPos=0;
      encoderInStream(encoderIn<pinA,pinB> &enc,int sensivity):enc(enc), sensivity(sensivity) {}
      inline void setSensivity(int s) {sensivity=s;}
      int available(void) {return abs(enc.pos-oldPos)/sensivity;}
      int peek(void) override {
        int d=enc.pos-oldPos;
        if (d<=-sensivity)return options->navCodes[downCmd].ch;
        if (d>=sensivity) return options->navCodes[upCmd].ch;
        return -1;
      }
      int read() override {
        int d=enc.pos-oldPos;
        if(d>0 && oldPos<0) d+=sensivity;
        else if(d<0 && oldPos>0) d-=sensivity;
        if (d<=-sensivity) {
          oldPos-=sensivity;
          return options->navCodes[downCmd].ch;
        }
        if (d>=sensivity) {
          oldPos+=sensivity;
          return options->navCodes[upCmd].ch;
        }
        return -1;
      }
      void flush() {oldPos=enc.pos;}
      size_t write(uint8_t v) {oldPos=v;return 1;}

    };

  }//namespace Menu
#endif
