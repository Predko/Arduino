#include <ModifiedMovingAverage.h>

template <typename T>
ModifiedMovingAverage<T>::ModifiedMovingAverage(unsigned nm) 
{  
  smoothingInterval = (nm == 0) ? 10 : nm;
  
  lastValue = 0;
}

template <typename T>
void ModifiedMovingAverage<T>::SetFirstValue(T fv)
{
  lastValue = fv;
}

template <typename T>
T ModifiedMovingAverage<T>::CalcNew(T newValue)
{
  lastValue = (newValue + lastValue * (smoothingInterval - 1)) / smoothingInterval;  
  
  return lastValue;
}

template <typename T>
inline T ModifiedMovingAverage<T>::Get()
{
  return lastValue;
}
