#include <iostream>			
#include <ws2tcpip.h>		//Contains definitions introduced in the WinSock 2 Protocol-Specific Annex document for TCP/IP that includes newer functions and structures used to retrieve IP addresses
#include <winsock2.h>
#include <string>
#include <sstream>
#include <thread>

#pragma comment (lib, "ws2_32.lib")  


//Project is small and done by a single person, so standard library is used
using std::cerr;
using std::getline;
using std::cin;
using std::thread;
using std::cout;
using std::endl;
using std::string;
using std::ostringstream;



//Thread to close server.
void stopServer(bool* runningPtr)
{
	cout << "Press enter to close server.\n";
	cin.get();
	*runningPtr = false;
}



void main()
{
	// Initialze winsock
	WSADATA wsData;				//Contains information about the Windows Sockets implementation
	WORD ver = MAKEWORD(2, 2);    //MAKEWORD concatenates the high and low bytes of what version entered to return a WORD value



	//Cannot find suitable WinSock DLL error
	int wsk = WSAStartup(ver, &wsData);		//Initiates use of the WinSock DLL with parameters of version and pointer to the Windows Sockets implementation
	if (wsk != 0)
	{
		cerr << "Can't find suitable DLL." << endl;
		return;
	}
	//WinSock DLL does not support 2.2+ error (extra precaution that can be removed to possibly be compatible with older versions of WinSock DLL)
	if (LOBYTE(wsData.wVersion) != 2 ||
		HIBYTE(wsData.wVersion) != 2) {
		cerr << "WinSock DLL does not support version 2.2+." << endl;
		WSACleanup();
		return;
	}


	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);  //Creates a listening socket by using "socket()" function with parameters of address family (AF_INET for IPv4 or AF_INET6 for IPv6), 
														 //specifications for socket (SOCK_STREAM can use TCP and two-way streams), and the protocol to be used (a value of 0 indicates that the
														 //service provider will choose)
	if (listening == INVALID_SOCKET)	
	{
		cerr << "Can't create a socket! Quitting" << endl;
		return;
	}



	// Bind the IP address and port to a socket
	sockaddr_in hint;			//Includes the members of sin_family (address family for the transport address that should always be set to AF_INET), sin_port (the transport protocol port number),
								//sin_addr (and IN_ADDR structure that contains an IPv4 trasport address), and sin_zero (for system use)
	hint.sin_family = AF_INET;
	hint.sin_port = htons(54000);	
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton  

	bind(listening, (sockaddr*)&hint, sizeof(hint));	//Binds hint's values to the listening socket



	// Tell Winsock the socket is for listening 
	listen(listening, SOMAXCONN);



	// Create the master file descriptor set and zero it
	fd_set master;		
	FD_ZERO(&master);



	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	FD_SET(listening, &master);



	// this will be changed by the \quit command 
	bool running = true;
	bool* runningPtr = &running;



	//Thread to choose to stop running the server
	thread first(stopServer, runningPtr);



	//Username list with capacity of 1024
	string usernameList [1024];



	while (running)
	{

		// Make a copy of the master file descriptor set, this is SUPER important because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages. 

		// E.g. You have a server and it's master file descriptor set contains 5 items;
		// the listening socket and four clients. When you pass this set into select(), 
		// only the sockets that are interacting with the server are returned. Let's say
		// only one client is sending a message at that time. The contents of 'copy' will
		// be one socket. You will have LOST all the other sockets.

		// SO MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

		//Wait time 
		timeval wait;
		wait.tv_sec = 0;  //Wait 0 second
		wait.tv_usec = 100;  //and 100 milliseconds

		fd_set copy = master;

		// See who's talking to us
		int socketCount = select(0, &copy, nullptr, nullptr, &wait);  //Returns the total number of socket handles that are contained in the fd_set structure and timesout for wait's values


		// Loop through all the current connections/potential connections
		for (int i = 0; i < socketCount; i++)
		{
			// Makes things easy for us doing this assignment
			SOCKET sock = copy.fd_array[i];

			// Is it an inbound communication?
			if (sock == listening)
			{
				// Accept a new connection
				SOCKET client = accept(listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);

				// Send a welcome message to the connected client
				string welcomeMsg = "Connected to server.\r\n";
				send(client, welcomeMsg.c_str(), welcomeMsg.size() + 1, 0);

			}
			else // It's an inbound message for chat 
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else 
				{
					// Check to see if it's a command. /username/ changes or initiates username
					if (buf[0] == '!')
					{
						// Is the command change username? 
						string cmd = string(buf, bytesIn);
						if (cmd.substr(0,10) == "!username!")
						{
							//Sets socket number to accord with username
							usernameList[sock] = cmd.substr(10, cmd.length() - 10);
							string h = usernameList[sock];
							usernameList[sock] = h.substr(0, h.length());
							cout << usernameList[sock] << " has joined the server.\n";
						}

						// Unknown command
						continue;
					}
					else
					{
						cout << buf;
						// Send message to other clients, and definiately NOT the listening socket


						for (int i = 0; i < master.fd_count; i++)
						{
							SOCKET outSock = master.fd_array[i];
							if (outSock != listening && outSock != sock && buf != "")
							{
								string strOut = usernameList[sock] + ": " + buf;
								//strOut = strOut.substr(0, strOut.length() - 8);
								cout << strOut.length();
								send(outSock, strOut.c_str(), strOut.size() + 1, 0);
							}
						}
					}
				}
			}
		}
	}



	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listening, &master);
	closesocket(listening);

	// Message to let users know what's happening and cleanup.
	string msg = "Server is shutting down. Goodbye\r\n";
	while (master.fd_count > 0)
	{
		// Get the socket number
		SOCKET sock = master.fd_array[0];

		// Send the goodbye message
		send(sock, msg.c_str(), msg.size() + 1, 0);

		// Remove it from the master file list and close the socket
		FD_CLR(sock, &master);
		closesocket(sock);
	}



	//Cleanup winsock
	WSACleanup();
	cout << "Server closed.\n";
	exit(EXIT_SUCCESS);  //Ends the thread
}