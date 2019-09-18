#ifndef AverageValue_h
#define AverageValue_h

#include <Arduino.h>

#define MAX_MEASUREMENTS 10

// экземпляр класса аккамулирует некоторое количество измерений
// (не больше MAX_MEASUREMENTS и равное указанному при создании).
// Возращает среднее значение за последние numberOfMeasurements измерений
class AverageValue 
{
public:
  AverageValue(int nm);  // nm <= MAX_MEASUREMENTS
  int Get();
  void Init();  
  void AddNext(int val);

private:
  int *values;
  int summ;
  int currentVal;
  int count;
  int numberOfMeasurements;
};

#endif