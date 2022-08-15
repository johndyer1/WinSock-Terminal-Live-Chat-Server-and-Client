#include <iostream>
#include <fstream>  //Streams chat to a file to allow for live conversation
#include <string>
#include <WS2tcpip.h>
#include <math.h>
#include <thread>
#include <stdlib.h>
#include <chrono> //for sleep() function
#pragma comment(lib, "ws2_32.lib")


//Project is small and done by a single person, so standard library is used
using std::cerr;
using std::endl;
using std::string;
using std::cout;
using std::getline;
using std::cin;
using std::thread;
using std::fstream;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, secondsusing namespace std;



//Can be made to update the screen with a live UI on virtual console 
//Can easily be used with a GUI
void liveChat(string incoming, string outgoing)
{
	
}



//Decrypting multithread loop to continue receiving messages while the user types
void receive(SOCKET sock, char buf[], string password)
{
	bool x = true; //Runs program until socket disconnects
	int lastBytesReceived = 0;

	while (x)
	{
		if (sock == SOCKET_ERROR) //Connection is closed, so end the loop
		{
			x = false;
		}
		else //Connection is still established
		{
			// Wait for response
			ZeroMemory(buf, 4096);
			sleep_for(nanoseconds(1));
			int bytesReceived = recv(sock, buf, 4096, 0);
			if (bytesReceived > 0 && lastBytesReceived != bytesReceived)
			{
				// Echo response to console 
				cout << string(buf, 0, bytesReceived) << endl;
			}
		}
	}
	exit(EXIT_SUCCESS);  //Ends the thread
}



void main()
{
	string ipAddress,			// IP Address of the server
		username,				//User-chosen username
		password,				//Password entered to join the server
		portString;				//Used to convert string port to int
	int port = 0;						// Listening port number 



	//Enter in user ip address (local server is 127.0.0.1), port number (local server is 54000), username, and password (it's "Art50")
	cout << "Enter the IP address: ";
	getline(cin, ipAddress);
	cout << "Enter the port number: ";
	getline(cin, portString);
	port = stoi(portString);
	cout << "Choose your username: ";
	getline(cin, username);



	// Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
		return;
	}



	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}



	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);



	// Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return;
	}



	// Do-while loop to send and receive data
	char buf[4096];
	string userInput;



	// Wait for response
	ZeroMemory(buf, 4096);
	int bytesReceived = recv(sock, buf, 4096, 0);
	if (bytesReceived > 0)
	{
		// Echo response to console
		cout << string(buf, 0, bytesReceived) << endl;
	}

	

	//Send username to server
	string firstUsername = "!username!" + username;
	int sendResult = send(sock, firstUsername.c_str(), firstUsername.size(), 0);
	if (sendResult == SOCKET_ERROR)
	{
		cout << "Could not send username!\n";
	}



	//Receive messages thread
	thread first(receive, sock, buf, password);



	//Encrypting send messages thread (Exit by hitting "Enter" with no message)
	do
	{
		// Prompt the user for some text
		getline(cin, userInput);
		const int messageLength = 10000 + userInput.length();
		//userInput = userInput + "|$#";
		//userInput = userInput + std::to_string(messageLength);

		if (userInput.size() > 0)		// Make sure the user has typed in something
		{
			// Send the text
			cout << userInput;
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
			if (sendResult == SOCKET_ERROR)
			{
				cout << "Could not send message!\n";
			}
		}

	} while (userInput.size() > 0);



	//Close down everything
	closesocket(sock);
	WSACleanup();
	exit(EXIT_SUCCESS);  //Ends the thread
}
