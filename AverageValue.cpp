#include "AverageValue.h"

// экземпляр класса аккамулирует некоторое количество измерений
// (не больше MAX_MEASUREMENTS и равное указанному при создании).
// Возращает среднее значение за последние numberOfMeasurements измерений

AverageValue::AverageValue(int nm) 
{  
  numberOfMeasurements = (nm > MAX_MEASUREMENTS) ? MAX_MEASUREMENTS : nm;

  values = new int[numberOfMeasurements];
  if(values == NULL)
  {
    numberOfMeasurements = 0; // недостаточно памяти для работы экземпляра
    Serial.println("недостаточно памяти для работы экземпляра");
  }

  Init(); 
}

void AverageValue::Init()
{
  if(values == NULL)
  {
    return;
  }

  summ = 0;
  count = 0;
  currentVal = 0;
  for(int i = 0; i != numberOfMeasurements; i++)
  {
    values[i] = 0;
  }
}

int AverageValue::Get()
{
  if(values == NULL || count == 0)
  {
    return INT16_MIN;
  }

  return summ / count;
}

void AverageValue::AddNext(int val)
{
  if(values == NULL)
  {
    return;
  }

  summ -= values[currentVal];
  
  summ += val;
  
  values[currentVal] = val;
  
  currentVal++;
  
  if(currentVal == numberOfMeasurements)
  {
    currentVal = 0;
  }
   
  if(count != numberOfMeasurements)
  {
    count++;
  }
}
