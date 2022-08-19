#include <iostream>
#include <fstream>  //Streams chat to a file to allow for live conversation
#include <string>
#include <WS2tcpip.h>
#include <math.h>
#include <bitset>
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
using std::bitset;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono; // nanoseconds, system_clock, secondsusing namespace std;



long long d1;  //Private key 1 for decrytion
long long d2;
long long elp1;  //Public key 
long long elp2;
long long n1;  //prime1 * primeB
long long n2;  //prime2 * primeA


/*
Below is a method of "criss-cross" double RSA encryption from a given password.  The steps are outlined below.

1. Take a given password and convert it to binary
2. Take every other bit and store in pass1 and take every bit left and put in pass2
3. Add a '1' in front of pass1 to break the symmetry that can occur when 2 different passwords have their pass1 equal the other's pass2 and vise-versa
4. Take pass1 and pass2 and convert them to integer values
5. Find and return the next consecutive prime integer after pass1, and calculate the difference between the two (prime1 - pass1 = diff1)
6. Do the same for pass2 and return prime2 and calculate diff2
7. Find and return the next consecutive prime integer after diff1 and return it as primeA
8. Do the same for diff2 and return primeB 

*/
//Find gcd
int gcd(int a, int h)
{
	int temp;
	while (true)
	{
		temp = a % h;
		if (temp == 0)
			return h;
		a = h;
		h = temp;
	}
}
//Converts string to array of ints
int toIntArray(string arr)
{
	string current = "";
	for (int i = 0; i < arr.length(); i++)
	{
		if (arr[i] != ' ')
		{
			current += arr[i];
		}
		else
		{
			return atoi(current.c_str());
		}
	}
}
//Returns the value for keys
long long keys(long long p1, long long p2, bool encry)
{
	//public key
	//e stands for encrypt
	long long totient = ((p1 - 1) * (p2 - 1));
	long long count;
	long long elp = 2;

	//for checking co-prime which satisfies e>1
	while (elp < totient) {
		count = gcd(elp, totient);
		if (count == 1)
			break;
		else
			elp++;
	}

	if (encry == true)
	{
		return (1 + (1 * totient)) / elp;
	}
	else
	{
		return elp;
	}

	return (1 + (123 * totient)) / elp;
}
//Finds next consecutive prime through a sieve
long nextPrime(long long p)
{
	while (true)
	{
		p++;
		if (p % 10 == 1 || p % 10 == 3 || p % 10 == 7 || p % 10 == 9)
		{
			for (int i = 3; i < p; i+=2)
			{
				if (p % i == 0)
				{
					break;
				}
				if (i >= p / 2)
				{
					return p;
				}
			}
		}
	}

}
//Creates the keys for RSA encryption and decryption
void createKeys(string password)
{
	string binPass,  //Used to store message in binary
		pass1,      //First loop of message
		pass2;    
	unsigned long long p1 = 0,
		p2 = 0,
		diff1,
		diff2,
		prime1,
		prime2,
		primeA,
		primeB,
		tt = 2;

	//Converts password string to binary
	for (int i = 0; i < password.length(); i++)
	{
		binPass += (bitset<8>(password.c_str()[i])).to_string();
	}

	//Takes every other bit of password and stores in pass1
	for (int i = 0; i < binPass.length(); i+=2)
	{
		pass1 += binPass.c_str()[i];
	}
	pass1 = "1" + pass1;  //Break symmetry
	//Convert pass1 to long p1 in reverse order (does not change security)
	for (int i = 0; i < pass1.length(); i++)
	{
		if (pass1[i] == '1')
		{
			p1 += pow(tt, i);
		}
	}

	//Takes every other bit of password and stores in pass2
	for (int i = 1; i < binPass.length(); i += 2)
	{
		pass2 += binPass.c_str()[i];
	}
	for (int i = 0; i < pass2.length(); i++)
	{
		if (pass2[i] == '1')
		{
			p2 += pow(tt, i);
		}
	}

	//Calculate prime values
	prime1 = nextPrime(p1);
	prime2 = nextPrime(p2);
	primeA = nextPrime(prime1 - p1);
	primeB = nextPrime(prime2 - p2);

	//Calculate n for both
	n1 = prime1 * primeB;
	n2 = prime2 * primeA;

	//Calculate encryption keys
	d1 = keys(prime1, primeB, true);
	d2 = keys(prime2, primeA, true);
	elp1 = keys(prime1, primeB, false);
	elp2 = keys(prime2, primeA, false);

}
//Encrypts using a simple algorithm
string encrypt(string mess)
{
	for (int i = 0; i < mess.length(); i++)
	{
		if (i % 2 == 0)
		{
			mess[i] = (long)mess[i] + 100;
		}
		else
		{
			mess[i] = (long)mess[i] + 150;
		}
	}
	return mess;
}
//Decrypts using a simple algorithm
string decrypt(string mess)
{
	for (int i = 0; i < mess.length(); i++)
	{
		if (i % 2 == 0)
		{
			mess[i] = (long)mess[i] - 100;
		}
		else
		{
			mess[i] = (long)mess[i] - 150;
		}
	}
	return mess;
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
				cout << decrypt(string(buf, 0, bytesReceived)) << endl;
			}
		}
	}
	exit(EXIT_SUCCESS);  //Ends the thread
}



void main()
{
	string ipAddress,			// IP Address of the server
		username,				//User-chosen username
		password,				//Password entered to encrypt messages
		portString;				//Used to convert string port to int
	int port = 0;						// Listening port number 


	//Enter in user ip address (local server is 127.0.0.1), port number (local server is 54000), username, and password (it's whatever you want to communicate with people who have the same password)
	cout << "Enter the IP address: ";
	getline(cin, ipAddress);
	cout << "Enter the port number: ";
	getline(cin, portString);
	port = stoi(portString);
	cout << "Choose your username: ";
	getline(cin, username);
	cout << "Enter the password (keep to around 7 characters to avoid long waits): ";
	getline(cin, password);
	createKeys(password);



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
		userInput = encrypt(userInput);
		const int messageLength = 10000 + userInput.length();
		//userInput = userInput + "|$#";
		//userInput = userInput + std::to_string(messageLength);

		if (userInput.size() > 0)		// Make sure the user has typed in something
		{
			// Send the text
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
