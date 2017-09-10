//********************************************************************************//
// read_dht11.c - reads data from dht11 sensor
// print temperature and humidity to output
// writes logs to /var/log/read_dht11.log
// executed by cron every 10 minutes
//********************************************************************************//
// includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wiringPi.h>
#include <time.h>

#define MAXTIMINGS	85
#define DHTPIN		7

int dht11_dat[5] = {0,0,0,0,0};
float humidity = 0.0;
float temperature = 0.0;

//********************************************************************************//
// GPIO related methods
int read_dht11() 
{
	uint8_t laststate	= HIGH;
	uint8_t counter		= 0;
	uint8_t j			= 0, i;
	//float	f; /* fahrenheit */
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

	/* pull pin down for 18 milliseconds */
	pinMode(DHTPIN, OUTPUT);
	digitalWrite(DHTPIN, LOW);
	delay(18);
	
	/* then pull it up for 40 microseconds */
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(40);
	
	/* prepare to read the pin */
	pinMode(DHTPIN, INPUT);	

	/* detect change and read data */
	for (i = 0; i < MAXTIMINGS; i++) 
	{
		counter = 0;
		while (digitalRead(DHTPIN) == laststate) 
		{
			counter++;
			delayMicroseconds(1);
			if (counter == 255)
				break;
		}
		laststate = digitalRead(DHTPIN);
		if (counter == 255)
			break;

		/* ignore first 3 transitions */
		if ((i >= 4) && (i % 2 == 0))	
		{
			/* shove each bit into the storage bytes */
			dht11_dat[j / 8] <<= 1;
			if (counter > 16)
				dht11_dat[j / 8] |= 1;
			j++;
		}
	}
	/*
	 * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	 * print it out if data is good
	 */
    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF))) 
    {
		//f = dht11_dat[2] * 9. / 5. + 32;
		char temp_buffer [30];
		sprintf(temp_buffer, "%d.%d", dht11_dat[0], dht11_dat[1]);
		humidity = atof(temp_buffer);
		sprintf(temp_buffer, "%d.%d", dht11_dat[2], dht11_dat[3]);
		temperature = atof(temp_buffer);
		//printf( "Humidity = %d.%d %% Temperature = %d.%d *C (%.1f *F)\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );
		return 0;
	} else {
		//printf( "Data not good, skip\n" );
		return -1;
	}
}

//********************************************************************************//
// main
int main(int argc, char **argv)
{
	time_t rawtime;
	struct tm *info;
	char buffer[80];
	
	time(&rawtime);

	// init GPIO Pins
	wiringPiSetup();
	
	// try to read data from the DHT11-sensor, give him 10 chances 
	int i = 0;
	while (i < 10) 
	{
		if (read_dht11() == 0) 
		{
			// reading complete
			info = localtime(&rawtime);
			strftime(buffer,80,"%d.%m.%Y %H:%M:%S", info);
			printf( "[%s] Temperature = %.1f *C | Humidity = %.1f %%\n", buffer, temperature, humidity);
			return 0;
		}
		i++;
		info = localtime(&rawtime);
		strftime(buffer,80,"%d.%m.%Y %H:%M:%S", info);
		printf("[%s] WARNING: Reading from DHT11 failed! (%d. try)\n", buffer, i);
		delay(2000); // wait 2 sec
	}
	
	// reading from DHT11 failed 10 times, sensor broken?
	info = localtime(&rawtime);
	strftime(buffer,80,"%d.%m.%Y %H:%M:%S", info);
	printf("[%s] ERROR: Couldnt read data from DHT11 sensor!\n", buffer);
	return 0;
}