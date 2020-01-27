#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;

// Want to send the intial message then do a loop of recv() until recv() is empty
void talk_to_server (int sockfd, char* server_name, char* port){
	char buf [1024];

	string server = server_name;
	string portNum = port;

	string extension = "/";
	size_t found = server.find("/");
	if(found != string::npos) // '/' was found and we need to get the root path from there
	{
		extension = server.substr(found, server.length()-found);
		server = server.substr(0, found);
	}

	string request = "GET " + extension + " HTTP/1.1\r\nHost: " + server + "\r\nConnection: close\r\n\r\n"; // Final total request

	int fd = open("received.html", O_CREAT | O_WRONLY | O_TRUNC, 0777);

	const char* c_request = request.c_str();
	int length = request.length();
	cout << "About to send following message: " << endl << c_request << endl;
	if (send (sockfd, c_request, length, 0) == -1){
            perror("client: Send failure");
            exit(0);
        }
	int numReceived = 1; // just to start the loop
	int numLoops = 0;
	int charStart = 0;
	while (numReceived > 0){
	    numReceived = recv(sockfd, buf, sizeof(buf), 0); // will write length of numReceived
	    if(numLoops == 0)
	    {
		    for(int i = 0; i < sizeof(buf); ++i)
		    {
			    if(buf[i] == '<')
			    {
				    charStart = i;
				    break;
			    }
		    }
		    //Write with a substring
		    int numToWrite = numReceived - charStart;
		    char* cNew = buf+charStart;
		    write(fd, cNew, numToWrite);
	    }
	    else // Write the whole string
	    {
	    	    write(fd, buf, numReceived);
	    }
	    memset(buf, 0, 1024);
	    numLoops++;
	}
	close(fd);
}



int client (char * server_name, char* port)
{
	struct addrinfo hints, *res;
	int sockfd;

	char origserver[512];
	strcpy(origserver, server_name);

	char server[512];
	bool hasExtension = false;
	int serverSize = strlen(server_name);
	for(int i = 0; i < serverSize; ++i)
	{
		if(server_name[i] == '/')
		{
			hasExtension = true;
			strncpy(server, server_name, i);
			break;
		}
	}
	// first, load up address structs with getaddrinfo():
	if(hasExtension)
	{
		strcpy(server_name, server);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	int status;
	//getaddrinfo("www.example.com", "3490", &hints, &res);
	if ((status = getaddrinfo(server_name, port, &hints, &res)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(status) << endl;
        return -1;
    }

	// make a socket:
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0)
	{
		perror ("Cannot create scoket");	
		return -1;
	}

	// connect!
	if (connect(sockfd, res->ai_addr, res->ai_addrlen)<0)
	{
		perror ("Cannot Connect");
		return -1;
	}
	cout << "Successfully connected to " << server_name << endl;
	/*cout <<"Now Attempting to send a message "<< server_name << endl;
	char buf [1024];
	sprintf (buf, "hello");
	send (sockfd, buf, strlen (buf)+1, 0);
	recv (sockfd, buf, 1024, 0);
	cout << "Received " << buf << " from the server" << endl;*/
	talk_to_server(sockfd, origserver, port);
	return 0;
}


int main (int ac, char** av)
{
	if (ac < 2){
        cout << "Usage: ./client <server name/IP>" << endl;
        exit (-1);
    }
	
	char port[] = "80";

	client (av [1], port);
}
