#include "EllersMazer.h"

EllersMazer mazer(20, 20);

void setup()
{
	Serial.begin(9600);
	mazer.build();
}

void loop()
{

  /* add main program code here */

}
