//Conner Michel
//2/14/17
//Professor Hoggard
//http://rhoggard.cs.ecu.edu/rhoggard/3010/assignments/assg01.aspx


#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include "FileHelper.h"
//using namespace std;

#pragma comment(lib, "Ws2_32.lib")

const char FILENAME[] = "data.bin";
const char IPADDR[] = "127.0.0.1";
const int  PORT = 50000;
const int  QUERY = 1;
const int  UPDATE = 2;

//--Prototypes-----------------------------------------------------------

// Closes the socket and performs the WSACleanup
void cleanup(SOCKET socket);

// Returns the version number from the data file
int getLocalVersion();

// Reads the two data values from the data file.
// When the function ends, num1 and num2 will be holding the
// two values that were read from the file.
void readData(int& num1, int& num2);

// Writes update to the data file.
// After execution data file will have updateToWrite 
// written over its contents.
void setData(int *updateToWrite, int size);

//Calculates the sum from data file.
//We dont return anything because the value is not being used.
void calculateSum(int localVersion);

// Creates a new socket and connects to server.
// returns socket to use.
SOCKET connectToServer(SOCKET mySocket);

//-----------------------------------------------------------------------

void cleanup(SOCKET socket)
{
	closesocket(socket);
	WSACleanup();
}

int getLocalVersion()
{
	std::ifstream dataFile;
	openInputFile(dataFile, FILENAME);

	int version = readInt(dataFile);
	dataFile.close();

	return version;
}

void readData(int& num1, int& num2)
{
	std::ifstream dataFile;
	openInputFile(dataFile, FILENAME);

	// Read the version number and discard it
	int tmp = num1 = readInt(dataFile);

	// Read the two data values
	num1 = readInt(dataFile);
	num2 = readInt(dataFile);

	dataFile.close();
}

void setData(int *updateToWrite, int size)
{
	std::ofstream dataFile;
	openOutputFile(dataFile, FILENAME);
	for (int i = 0; i < size; i++)
	{

		writeInt(dataFile, updateToWrite[i]);
		dataFile.flush();
	}
	dataFile.close();
}

void calculateSum(int localVersion) {
	int			sum;
	int			num1 = 0;
	int			num2 = 0;

	std::cout << "\nSum Calculator Version " << localVersion << "\n\n";

	readData(num1, num2);
	sum = num1 + num2;
	std::cout << "The sum of " << num1 << " and " << num2 << " is " << sum << endl;
}

SOCKET connectToServer(SOCKET mySocket)
{

	SOCKADDR_IN	serverAddr;

	// Create a new socket for communication
	mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (mySocket == INVALID_SOCKET)
	{
		std::cerr << "ERROR: Cannot create socket\n";
		WSACleanup();
		return 1;
	}

	// Setup a SOCKADDR_IN structure which will be used to hold address
	// and port information for the server. Notice that the port must be converted
	// from host byte order to network byte order.
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, IPADDR, &serverAddr.sin_addr);

	//serverAddr.sin_addr.s_addr = inet_addr( ipAddress );   <== deprecated

	// Try to connect to server
	if (connect(mySocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << "ERROR: Failed to connect\n";
		cleanup(mySocket);
		return 1;
	}

	return mySocket;
}

int main()
{
	WSADATA		wsaData;
	SOCKET		mySocket = 0;
	int			localVersion = 0;
	int			serverVersion = 0;
	bool		upToDateFile = FALSE;	//FALSE until proven TRUE
	int			updateToWrite[3] = { 0, 0, 0 };


	// 1) make sure that we are using the current version of the data file
	localVersion = getLocalVersion();


	// Loads Windows DLL (Winsock version 2.2) used in network programming
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		std::cerr << "ERROR: Problem with WSAStartup\n";
		return 1;
	}

	std::cout << "\nChecking for updates...\n";

	//We will now connect to the server.
	mySocket = connectToServer(mySocket);


	//We will now send a request for the current version number.
	int iSend = send(mySocket, (char*)&QUERY, sizeof(int), 0);
	if (iSend == SOCKET_ERROR)
	{
		std::cerr << "ERROR: Failed to send message\n";
		cleanup(mySocket);
		return 1;
	}


	//We will now receive the version number from the server.
	recv(mySocket, (char*)&serverVersion, sizeof(int), 0);


	//We will now check if local version is same as received version.
	//yes? continue with program from local version
	if (serverVersion == localVersion)
	{
		upToDateFile = TRUE; //I feel this may help increase modularity later
	}

	//no? continue with update process
	else if (serverVersion != localVersion)
	{
		upToDateFile = FALSE; //I feel this may help increase modularity later
	}

	//?garbage error, cleanup , return 0
	else
	{
		std::cerr << "ERROR: BSoD,RRoD,YLoD...Update didn't go too well. Please try again.\n";
		cleanup(mySocket);
		return 0;
	}

	//We know if we are up to date or not so we can close connection.
	closesocket(mySocket);//close the socket but dont unload DLL as we may need it later.


	// 2) update the data file if it is out of date
	//We will now update our localVersion if needed.
	if (!upToDateFile)
	{
		//We will now connect to the server again.
		mySocket = connectToServer(mySocket);


		//We will now send a request for the updated file.
		int iSend = send(mySocket, (char*)&UPDATE, sizeof(int), 0);
		if (iSend == SOCKET_ERROR)
		{
			std::cerr << "ERROR: Failed to send message\n";
			cleanup(mySocket);
			return 1;
		}


		//We will now receive the update as 3 separate ints.
		std::cout << "\nDownloading updates...\n";
		for (int i = 0; i < 3; i++)
		{
			recv(mySocket, (char*)&updateToWrite[i], sizeof(int), 0);
			//std::cout << updateToWrite[i] << "\n"; //Check what we just got

		}
		//Of course it works...I'm the best. pshhhhhhhhhhhhhh...

		//We have now received 3 ints and can close our connection.
		cleanup(mySocket);//now we can close the socket and unload DLL.


		//We will now open our file, delete its contents, and write updated data.
		setData(updateToWrite, sizeof(updateToWrite) / sizeof(*updateToWrite));

		std::cout << "Update finished";
	}
	//We are done with the server and can now unload the DLL.
	cleanup(mySocket);

	// Main purpose of the program starts here: read two numbers from the data file and calculate the sum
	localVersion = getLocalVersion(); //This is called again because we may have updated localVersion.

	//We will now calculate the sum from our data file.
	calculateSum(localVersion);

	return 0;//end
}
