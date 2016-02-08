/*Arinc Elhan
1819291
Section 1*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

enum DIRECTION {LANDING, TAKEOFF};
pthread_mutex_t mutexWrite;// This mutex is used for atomic write operation
void writeToFile(int mode,int flightCode, DIRECTION direction, int runwayId,int parkId,int taxiNodeId)// This function is used to write the results of simulation to the file
{
	// Set the appropriate parameters for the corresponding output

	pthread_mutex_lock(&mutexWrite);

	if(mode == 0)//This will be output when the capacity of the airport is available for the new plane
	{
		printf ("Plane = %d started %s\n",flightCode, (direction) ? "take-off" : "landing");
	}
	else if(mode == 1)//This will be output when the corresponding runway is available and the landing starts
	{
		printf("Plane = %d started its landing on runway %d\n",flightCode,runwayId);
	}
	else if(mode == 2)//This will be output when the corresponding runway is available and the take-off starts
	{
		printf("Plane = %d started its take off from runway %d\n",flightCode,runwayId);
	}
	else if(mode == 3)//This will be output when the current landing operation on the corresponding runway finished
	{
		printf("Plane = %d finished its landing on runway %d\n",flightCode,runwayId);
	}
	else if(mode == 4)//This will be output when the current take-off operation on the corresponding runway finished
	{
		printf("Plane = %d finished its take off from runway %d\n",flightCode,runwayId);
	}
	else if(mode == 5)//This will be output when reaching to the corresponding taxi node finished
	{
		printf("Plane = %d entered to taxi node %d\n",flightCode,taxiNodeId);		
	}
	else if(mode == 6)//This will be output when the taxying to the corresponding park finished
	{
		printf("Plane = %d finished its taxying to the park %d\n",flightCode,parkId);
	}
	else if(mode == 7)//This will be output when the taxying to the corresponding runway finished
	{
		printf("Plane = %d finished its taxying to the runway %d\n",flightCode,runwayId);
	}

	pthread_mutex_unlock(&mutexWrite);
}

struct taxiNodeStr
{
	int taxiNodeId;
	pthread_mutex_t taxiMutex; 
};

struct runwayStr
{
	int runwayId;
	vector<int> connectedParks;
	vector< vector<taxiNodeStr*> > connectedParkPaths;
	pthread_mutex_t runwayMutex; 
};

struct parkStr
{
	int parkId;
	vector<int> connectedRunways;
	vector< vector<taxiNodeStr*> > connectedRunwayPaths;
};

struct flightStr
{
	int flightId;
	string flightType;
	int parkId;
	int runwayId;
};

vector<runwayStr*> runways;
vector<parkStr*> parks;
vector<flightStr*> planes;
vector<taxiNodeStr*> taxis;

sem_t sem;
pthread_attr_t attr;
int counter;

void *threadfunc(void *data);

pthread_mutex_t safetyMutex;
pthread_mutex_t safetyMutex2;

int main(int argc, char *argv[])
{
	ifstream myReadFile;
    	myReadFile.open(argv[1]);
	string line1;
	string line2;
	string temp;
	string whichway;
	int i = 0;
	int k = 0;
	int runwayNum;
	int parkNum;
	int taxinodeNum;
	int planeCapacity;
	int yol;
	int whichrunway;
	int whichparkway;
	pthread_mutex_init(&safetyMutex,NULL);
	pthread_mutex_init(&safetyMutex2,NULL);
	if (myReadFile.is_open())
    	{
    	getline(myReadFile,line1);
    	runwayNum = atoi(line1.c_str());
    	for (i = 1; i <= runwayNum; i++)
    	{
    		runwayStr* runway = new runwayStr;
    		runway->runwayId = i;
    		pthread_mutex_init(&runway->runwayMutex,NULL);
    		runways.push_back(runway);
    	}
    	getline(myReadFile,line1);
    	parkNum = atoi(line1.c_str());
    	for (i = 1; i <= parkNum; i++)
    	{
    		parkStr* park = new parkStr;
    		park->parkId = i;
    		parks.push_back(park);
    	}
    	getline(myReadFile,line1);
    	taxinodeNum = atoi(line1.c_str());
    	for (i = 1; i <= taxinodeNum; i++)
    	{
    		taxiNodeStr* globalTaxi = new taxiNodeStr;
    		globalTaxi->taxiNodeId = i;
    		pthread_mutex_init(&globalTaxi->taxiMutex,NULL);
    		taxis.push_back(globalTaxi);
    	}
    	getline(myReadFile,line1);
    	planeCapacity = atoi(line1.c_str());
    	
    	while (getline(myReadFile, line1))
    	{
    		if (line1 == "-1")
    		{
    			break;
    		}
    		else
    		{
    			istringstream f(line1);
            	getline(f, temp, ' ');
            	k = 0;
            	if (temp[k] == 'R')
            		whichway = "Runway";
            	else
            		whichway = "Parkway";
            	for (k = 1; k < temp.size(); k++)
            	{
            		if (temp[k] > 47 && temp[k] < 58)
            		{
            			continue;
            		}
            		else
            		{
            			break;
            		}
            	}
            	if (whichway == "Runway")
            	{
            		whichrunway  = atoi((temp.substr(1,k-1)).c_str());
            		whichparkway = atoi((temp.substr(k+1)).c_str());
            	}
            	else
            	{
            		whichrunway  = atoi((temp.substr(k+1)).c_str());
            		whichparkway = atoi((temp.substr(1,k-1)).c_str());
            	}

				vector<taxiNodeStr*> tempvector;

            	while (1)
            	{
            		getline(f, temp, ' ');
            		if (temp == "-1")
            		{
            			break;
            		}
            		for (int u = 0; u < taxis.size(); u++)
            		{
            			if (taxis[u]->taxiNodeId == atoi(temp.c_str()))
            			{
            				tempvector.push_back(taxis[u]);
            			}
            		}
            	}

            	if ( whichway == "Runway")
            	{

            		runways[whichrunway-1]->connectedParks.push_back(whichparkway); // 1 EKSIK DEGER GEREK ARRAYE ATMAK ICIN
            		runways[whichrunway-1]->connectedParkPaths.push_back(tempvector);
            	}
            	else
            	{
            		parks[whichparkway-1]->connectedRunways.push_back(whichrunway);
            		parks[whichparkway-1]->connectedRunwayPaths.push_back(tempvector);
            	}
    		}
    	}
    }
    myReadFile.close();
    
   	int fID;
   	int pID;
   	int rID;
   	string fType;

   	sem_init(&sem,0,planeCapacity);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

	counter = 0;
	while (getline(cin, line2))
	{
   		  istringstream f(line2);
            getline(f, temp, ' ');
            fID = atoi(temp.c_str());
            getline(f, temp, ' ');
            fType = temp;
            getline(f, temp, ' ');
            rID = atoi(temp.c_str());
            getline(f, temp, ' ');
            pID = atoi(temp.c_str());
            flightStr* data = new flightStr;
            data->flightId = fID;
            data->runwayId = rID;
            data->parkId = pID;
            data->flightType = fType;
            planes.push_back(data);

			pthread_t thr;
			counter++;
			pthread_create(&thr, &attr, threadfunc, (void *) data);	
	}
	pthread_attr_destroy(&attr);
	while(counter != 0)
	{
		usleep(1);
	}
	return 0;
}

void *threadfunc(void *data)
{
	sem_wait(&sem);
	int a,b,c,d;
	flightStr *fr;
	fr = (flightStr*)data;

	if (fr->flightType == "L")
	{
		writeToFile(0,fr->flightId,LANDING,fr->runwayId,fr->parkId,0);
		
		pthread_mutex_lock(&runways[(fr->runwayId)-1]->runwayMutex);
		writeToFile(1,fr->flightId,LANDING,fr->runwayId,fr->parkId,0);
		usleep(10000);
		writeToFile(3,fr->flightId,LANDING,fr->runwayId,fr->parkId,0);
		pthread_mutex_unlock(&runways[(fr->runwayId)-1]->runwayMutex);

		int runwayAddress = (fr->runwayId);
		int parkAddress;

		pthread_mutex_lock(&safetyMutex);
		a = 0;
		b = 0;
		for (a = 0; a < runways[runwayAddress-1]->connectedParks.size(); a++)
		{
			if (runways[runwayAddress-1]->connectedParks[a] == fr->parkId)
			{
				parkAddress = a+1;
			}
		}
		pthread_mutex_unlock(&safetyMutex);
		
		pthread_mutex_lock(&runways[runwayAddress-1]->connectedParkPaths[parkAddress-1][0]->taxiMutex);
		writeToFile(5,fr->flightId,LANDING,fr->runwayId,fr->parkId,
			runways[runwayAddress-1]->connectedParkPaths[parkAddress-1][0]->taxiNodeId);
		usleep(10000);


		for (b = 1; b < runways[runwayAddress-1]->connectedParkPaths[parkAddress-1].size(); b++)
		{
			pthread_mutex_lock(&runways[runwayAddress-1]->connectedParkPaths[parkAddress-1][b]->taxiMutex);
			writeToFile(5,fr->flightId,LANDING,fr->runwayId,fr->parkId,
				runways[runwayAddress-1]->connectedParkPaths[parkAddress-1][b]->taxiNodeId);
			pthread_mutex_unlock(&runways[runwayAddress-1]->connectedParkPaths[parkAddress-1][b-1]->taxiMutex);
			usleep(10000);
			if ( b == runways[runwayAddress-1]->connectedParkPaths[parkAddress-1].size() - 1)
			{
				writeToFile(6,fr->flightId,LANDING,fr->runwayId,fr->parkId,0);
				pthread_mutex_unlock(&runways[runwayAddress-1]->connectedParkPaths[parkAddress-1][b]->taxiMutex);
			}
		}
	}
	else if (fr->flightType == "T")
	{
		writeToFile(0,fr->flightId,TAKEOFF,fr->runwayId,fr->parkId,0);
		int parkAddress = (fr->parkId);
		int runwayAddress;

		pthread_mutex_lock(&safetyMutex2);
		c = 0;
		d = 0;
		for (c = 0; c < parks[parkAddress-1]->connectedRunways.size(); c++)
		{
			if (parks[parkAddress-1]->connectedRunways[c] == fr->runwayId)
			{
				runwayAddress = c+1;
			}
		}
		pthread_mutex_unlock(&safetyMutex2);

		pthread_mutex_lock(&parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1][0]->taxiMutex);
		writeToFile(5,fr->flightId,TAKEOFF,fr->runwayId,fr->parkId,
			parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1][0]->taxiNodeId);
		usleep(10000);

		for (d = 1; d < parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1].size(); d++)
		{
			pthread_mutex_lock(&parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1][d]->taxiMutex);
			writeToFile(5,fr->flightId,TAKEOFF,fr->runwayId,fr->parkId,
				parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1][d]->taxiNodeId);
			pthread_mutex_unlock(&parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1][d-1]->taxiMutex);
			usleep(10000);
			if ( d == parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1].size() - 1)
			{
				writeToFile(7,fr->flightId,TAKEOFF,fr->runwayId,fr->parkId,0);
				pthread_mutex_unlock(&parks[parkAddress-1]->connectedRunwayPaths[runwayAddress-1][d]->taxiMutex);
			}
		}
		
		pthread_mutex_lock(&runways[(fr->runwayId)-1]->runwayMutex);
		writeToFile(2,fr->flightId,LANDING,fr->runwayId,fr->parkId,0);
		usleep(50000);
		writeToFile(4,fr->flightId,LANDING,fr->runwayId,fr->parkId,0);
		pthread_mutex_unlock(&runways[(fr->runwayId)-1]->runwayMutex);
	}
	counter--;
	sem_post(&sem);
	pthread_exit((void*) data);
}
