#ifndef SplitedString_h
#define SplitedString_h

#include <Arduino.h>

/*
Разбиение строки(char *s) на подстроки 
с указанным char dl разделяющим символом.
Формат получаемой строки: [nextSubStrAdr(1 byte),...SubStr...,0(завершение подстроки),nextSubStrAdr(1 byte),...]
getValue(uint8_t numb_substriing), возвращает подстроку(String) 
с указанным номером или пустую ("") строку. 
*/

#define MAX_SUBSTR 40

class SplitedString {
  private:
          char  str[100];
          char  dlmt;
          uint8_t current,
                  posLenght;    // адресс байта с длиной подстроки.
          uint8_t count;

  public:
          SplitedString(char d = ',');

          void Reset() { *str = 0;}
          void AddChar(char ch);

          char *getValue(uint8_t pos);

          void  operator+=(char ch) { AddChar(ch); }

          char GetFirstChar(uint8_t numbSubStr);
};

#endif