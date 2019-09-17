#include "AverageValue.h"

// экземпляр класса аккамулирует некоторое количество измерений
// (не больше MAX_MEASUREMENTS и равное указанному при создании).
// Возращает среднее значение за последние numberOfMeasurements измерений

AverageValue::AverageValue(int nm) 
{  
  numberOfMeasurements = (nm > MAX_MEASUREMENTS) ? MAX_MEASUREMENTS : nm;

  values = new float[numberOfMeasurements];
  if(values == NULL)
  {
    numberOfMeasurements = 0; // недостаточно памяти для работы экземпляра
    Serial.println("недостаточно памяти для работы экземпляра");
  }

  Init(); 
}

void AverageValue::Init()
{
  summ = 0;
  count = 0;
  currentVal = 0;
  for(int i = 0; i != numberOfMeasurements; i++)
  {
    values[i] = 0;
  }
}

float AverageValue::Get()
{
  return (count != 0) ? summ / count : NAN;
}

void AverageValue::AddNext(float val)
{
  if(val == NAN)
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
