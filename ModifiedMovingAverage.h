#pragma once

#include <arduino.h>


template <class T> 
class ModifiedMovingAverage 
{
public:
  // nm - число измерений
  ModifiedMovingAverage(unsigned nm);
  
  // Устанавливает первое значение
  void SetFirstValue(T fv);

  // Вычисляет новое значение
  T CalcNew(T newValue);

  inline T Get();

private:
  // Последннее значение.
  T lastValue;

  // Интервал измерений
  unsigned smoothingInterval;
};
