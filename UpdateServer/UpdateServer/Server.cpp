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

int main()
{
	// Add your code here for the server
	WSADATA		wsaData;
	SOCKET		listenSocket;
	SOCKET		acceptSocket;
	SOCKADDR_IN	serverAddr;
	int			localVersion = 0;
	int			whatDoesMyClientWant = 0;
	int			requestsThisSession = 0;
	int			toSend[3] = { 0, 0, 0 };

	localVersion = getLocalVersion();

	std::cout << "\nUpdate server\n";
	std::cout << "Data file version " << localVersion << endl;
	std::cout << "Running on port number " << PORT << endl;

	// Loads Windows DLL (Winsock version 2.2) used in network programming
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		std::cerr << "ERROR: Problem with WSAStartup\n";
		return 1;
	}
	do {
		// Create a new socket to listen for client connections
		listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (listenSocket == INVALID_SOCKET)
		{
			std::cerr << "ERROR: Cannot create socket\n";
			WSACleanup();
			return 1;
		}

		// Setup a SOCKADDR_IN structure which will be used to hold address
		// and port information. Notice that the port must be converted
		// from host byte order to network byte order.
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(PORT);
		inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

		//serverAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );   <== deprecated

		// Attempt to bind to the port.
		if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			std::cerr << "ERROR: Cannot bind to port\n";
			cleanup(listenSocket);
			return 1;
		}

		// Start listening for incoming connections
		if (listen(listenSocket, 1) == SOCKET_ERROR)
		{
			std::cerr << "ERROR: Problem with listening on socket\n";
			cleanup(listenSocket);
			return 1;
		}

		std::cout << "\nWaiting for connections...\n";

		// Accept incoming connection.  Program pauses here until
		// a connection arrives.
		acceptSocket = accept(listenSocket, NULL, NULL);

		// For this program, the listen socket is no longer needed so it will be closed
		closesocket(listenSocket);

		std::cout << "Connection received\n";

		//receive connection
		int iRecv = recv(acceptSocket, (char*)&whatDoesMyClientWant, sizeof(whatDoesMyClientWant), 0);

		//request for current version
		if (whatDoesMyClientWant == 1)
		{
			std::cout << "\tRequest for current version number\n";
			//send current version number
			int iSend = send(acceptSocket, (char*)&localVersion, sizeof(localVersion), 0);
			if (iSend == SOCKET_ERROR)
			{
				std::cerr << "ERROR: Failed to send message\n";
				cleanup(acceptSocket);
				return 1;
			}
			//std::cout << "pass request\n";

		}
		//request for update
		else if (whatDoesMyClientWant == 2)
		{
			std::cout << "\tRequest for update\n";
			//prepare an array
			toSend[0] = localVersion;
			readData(toSend[1], toSend[2]);
			//send update

			for (int i = 0; i < sizeof(toSend) / sizeof(*toSend); i++)
			{
				int iSend = send(acceptSocket, (char*)&toSend[i], sizeof(toSend[i]), 0);
				if (iSend == SOCKET_ERROR)
				{
					std::cerr << "ERROR: Failed to send message\n";
					cleanup(acceptSocket);
					return 1;
				}
			}
			//std::cout << "pass update\n";
		}
		else
		{
			std::cerr << "ERROR: Unexpected transmission\n";
			cleanup(acceptSocket);
			return 1;
		}
		closesocket(acceptSocket);

		std::cout << "\tConnection closed\n";
		std::cout << "Total requests handled : " << ++requestsThisSession << endl;
	} while (1);//exectutive decision. This program will never end.

	//end
	cleanup(acceptSocket);//listenSocket has already been closed
	return 0;
}
