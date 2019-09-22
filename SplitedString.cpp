#include "SplitedString.h"

/*
Разбиение строки(char *s) на подстроки 
с указанными в виде строки(char *dl) разделяющими символами.
В экземпляре класса хранится ссылка на строку
 и массив смещений от начала строки на подстроки.
getValue(uint8_t numb_substriing), возвращает подстроку(String) 
с указанным номером или пустую ("") строку. 
*/


SplitedString::SplitedString(char dl)
{
	dlmt = dl;
	Reset();
}

void SplitedString::Reset() 
{ 
	posLenght = 0;
	for (uint8_t i = 0; i != 100; i++)
	{
		str[i] = 0;
	}

	current = 1;
}

void SplitedString::AddChar(char ch)
{
  if(ch != dlmt)
  {
    str[current] = ch;
    
    str[++current] = 0;
    
    str[posLenght]++;
  }
  else
  { // переходим к следующей подстроке
    current++;

    posLenght = current;

    str[posLenght] = 0;

    current++;
  }
}


char *SplitedString::getValue(uint8_t numb_substr)
{
  char *s = str;
  for(uint8_t i = 0; numb_substr != i && *s != 0; i++)
  {
    s += *s + 2;  // К следующей подстроке 2 = (1 byte - lenght) + (1 byte - 0 (endstr))
  }
  
  if( *s == 0)
  {
    return NULL;  // строка пустая или у неё нет такой подстроки
  }
  //Serial.print("getValue = "); Serial.println(s+1);
  return s + 1; // первый байт - длина подстроки, поэтому указываем на следующий
}

char SplitedString::GetFirstChar(uint8_t numbSubStr)
{
  return *(getValue(numbSubStr));
}

