#include<iostream>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace std;
string trim (string input){
    int i=0;
    while (i < input.size() && input [i] == ' ')
        i++;
    if (i < input.size())
        input = input.substr (i);
    else{
        return "";
    }
    
    i = input.size() - 1;
    while (i>=0 && input[i] == ' ')
        i--;
    if (i >= 0)
        input = input.substr (0, i+1);
    else
        return "";
    
    return input;
    

}

vector<string> split (string line, string separator=" "){
    vector<string> result;
    while (line.size()){
        size_t found = line.find(separator);
        if (found == string::npos){
            string lastpart = trim (line);
            if (lastpart.size()>0){
                result.push_back(lastpart);
            }
            break;
        }
        string segment = trim (line.substr(0, found));
        //cout << "line: " << line << "found: " << found << endl;
        line = line.substr (found+1);

        //cout << "[" << segment << "]"<< endl;
        if (segment.size() != 0) 
            result.push_back (segment);

        
        //cout << line << endl;
    }
    return result;
}

char** vec_to_char_array (vector<string> parts){
    char ** result = new char * [parts.size() + 1]; // add 1 for the NULL at the end
    for (int i=0; i<parts.size(); i++){
        // allocate a big enough string
        result [i] = new char [parts [i].size() + 1]; // add 1 for the NULL byte
        strcpy (result [i], parts[i].c_str());
    }
    result [parts.size()] = NULL;
    return result;
}

void execute (string command){
    vector<string> argstrings = split (command, " "); // split the command into space-separated parts
    char** args = vec_to_char_array (argstrings);// convert vec<string> into an array of char*
    execvp (args[0], args);
}

string stripString(string command) // Defined to strip the string of its quotes
{
	char pipeChar = '|' + 100;
	for(int i = 0; i < command.length(); ++i)
	{
		if((command[i] == '"') || (command[i] == '\''))
		{
			command.erase(i, 1); // Erase one character at position quotes
		}
		if(command[i] == pipeChar)
			command[i] = '|';
	}
	size_t temp = command.find("echo");
	if(temp != string::npos)
	{
		command.erase(temp, 4);
		for(int i = temp; i < command.size()-temp; ++i)
		{
			if(command[i] != ' ')
			{
				command.erase(temp, i);
				break;
			}	
		}
	}
	return command;
}

// STILL TODO
// input stream

int main (){
    char pipeChar = '|'+100; string stringPipeChar = "|" + 100;
    char inputChar = '<'+100;
    char outputChar = '>'+100;
    char fileName[100];
    vector<int> pids;
    vector<string> pidNames;

    time_t now =time(0);
    char* dt = ctime(&now);
    cout << "Current time: " << dt;

    while (true){ // repeat this loop until the user presses Ctrl + C
	int pipes = 0;
	bool inputs = false, outputs = false;
	bool instring = false;
	bool isEcho = false;
	bool isCD = false;
	bool isBG = false;
	bool isJobs = false;
	memset(&fileName[0], 0, 100);

        string commandline = "";/*get from STDIN, e.g., "ls  -la |   grep Jul  | grep . | grep .cpp" */
	// Need to parse and look for special characters | < and >
	char currDirectory[100];
	char pastDirectory[100];
	getcwd(currDirectory, 100);

	cout << "shell:" << currDirectory << "$ ";
	getline(cin, commandline);
	if(commandline.find("echo") != string::npos)
		isEcho =true;
	if(commandline.find("cd") != string::npos)
		isCD = true;
	if(commandline.find("&") != string::npos)
		isBG = true;
	if(commandline.find("jobs") != string::npos)
		isJobs = true;
	for(int i = 0; i < commandline.length(); ++i) // This will find all of the legal pipes, inputs, and outputs so they can be dealt with properly
	{
		if((commandline[i] == '"') || (commandline[i] == '\''))
		{
			if(instring)
				instring =false;
			else
				instring = true;
		}
		if((commandline[i] == '|') && (instring))
		{
			pipes++;
			commandline[i]+=100;
		}
		else if((commandline[i] == '<') && (!instring))
		{
			inputs = true;
			commandline[i]+=100;
			for(int j=i+1; j < commandline.length(); ++j) // to find filename
			{
				if((commandline[j] != ' ') && (commandline[j] != '"'))
				{
					int iter=0;
					while(true)
					{
						if((commandline[j] == ' ') || (commandline[j] == '"') || (j == commandline.length()))
						{
							break;
						}
						else
						{
							fileName[iter] = commandline[j];
							j++; iter++;
						}
					}
					break;
				}
			}
			//cout << "Filename is: " << fileName << endl;
		}
		else if((commandline[i] == '>') && (!instring))
		{
			outputs = true;
			commandline[i]+=100;
			for(int j=i+1; j < commandline.length(); ++j) // to find filename
			{
				if((commandline[j] != ' ') && (commandline[j] != '"'))
				{
					int iter=0;
					while(true)
					{
						if((commandline[j] == ' ') || (commandline[j] == '"') || (j == commandline.length()))
						{
							break;
						}
						else
						{
							fileName[iter] = commandline[j];
							j++; iter++;
						}
					}
					break;
				}
			}
			//cout << "Filename is: " << fileName << endl;

		}
	}
	// Let's strip the quotes out of the string
	// cout << "Pipes: " << pipes << endl << "Inputs: " << inputs << endl << "Outputs: "<< outputs << endl;
        // split the command by the "|", which tells you the pipe levels
	// TODO piping will break upon having pieps in the string still
        vector<string> tparts = split (commandline, "|");
	//cout << "Tparts size: " << tparts.size() << endl;
	int origIn = dup(0);
	int origOut = dup(1);

        // for each pipe, do the following:
	// TODO Fix the piping thing
        for (int i=0; i<tparts.size(); i++){
	    // Fixing string argument
	    if(!isEcho)
	    {
	    for(int j = 0; j < tparts[i].length(); ++j)
	    {
		    if(tparts[i][j] == ' ') // Now we want to strip quotes and spaces
		    {
			    ++j;
			    for(int k =j; k < tparts[i].length(); ++k)
			    {
				    if((tparts[i][k] == '\'') || (tparts[i][k] == '"')) //|| (tparts[i][k] == ' '))
				    {
					    //if(i < tparts[i].length()-2)
					    //{
					//	    if((tparts[i][k+1] != '-') || (tparts[i][k+1] != '.'))
					//	    {
					    		tparts[i].erase(k, 1);
					//	    }
					 //   }
				    }
				    if(tparts[i][k] == '{')
				    {
					    int origK =k;
					    while(tparts[i][k] != '}')
					    {
						    if(tparts[i][k] == ' ')
							    tparts[i].erase(k, 1);
						    k++;
					    }
					    k = origK;
				    }
			    }
			    break;
		    }
	    }
	    }
	    //cout << "COMMAND IS: " << tparts[i] << endl;
            // make pipe
	    int fd[2];
	    pipe(fd);
	    if(isBG)
		pidNames.push_back(tparts[i]);
	    int pid = fork();
    	    if (!pid){
                // redirect output to the next level
 		
                // unless this is the last level
		
                if (i < tparts.size() - 1){
		    dup2(fd[1], 1); // Now STDOUT is in fd[1]
                    // redirect STDOUT to fd[1], so that it can write to the other side
		    // Now write into the pipe so it can be pulled out
                    close (fd[1]);   // STDOUT already points fd[1], which can be closed
                }
		
                //execute function that can split the command by spaces to 
                // find out all the arguments, see the definition
		if(i == 0)
		{
			if(inputs)
			{
				int fileD = open(fileName, O_RDONLY, S_IRUSR | S_IWUSR);
				dup2(fileD, 0); // connect stdin to fd
				for(int j = 0; j < tparts[i].length(); ++j)
				{
					if(tparts[i][j] == inputChar)
					{
						tparts[i] = tparts[i].substr(0, j);
						break;
					}
				}
				/*char s[50];
				read(0, s, 50);
				cout << "File contains : " << s << endl << endl;*/
			//	cout << "Doing input." <<endl;
			}
		}
		if(i == tparts.size()-1) // last iteration check for output to file
		{
			if(outputs)
			{
				int fileD = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				dup2(fileD, 1); // redirect stdout to fd
				// Also want to fix the command vector so there isn't "> test.txt" at the end of the command
				for(int j = 0; j < tparts[i].length(); ++j)
				{
					if(tparts[i][j] == outputChar)
					{
						tparts[i] = tparts[i].substr(0, j);
					}
				}
			//	cout << "Doing output." <<endl;
			}
		}
		//cout << "Executing the following: ";
		//cout << tparts[i] << endl;
		if(isCD)
		{
			char directory[100];
			for(int j = 2; j < tparts[i].length(); ++j)
			{
				if(tparts[i][j] != ' ')
				{
					//directory = tparts[i].substr(j, tparts[i].length());
					tparts[i] = tparts[i].substr(j, tparts[i].length()); // new string
					strcpy(directory, tparts[i].c_str());
					break;
				}
			}
			if(directory[0] == '-')
				chdir(pastDirectory);
			else
			{
				getcwd(pastDirectory, 100);
				chdir(directory);
			}
		}
		else if(isJobs)
		{
			for(int j = 0; j < pids.size(); ++j)
			{
				cout << "Running\tPID: " << pids.at(j) << "\t" << pidNames.at(j) << endl;
			}
			exit(0);
		}
		else if(isBG)
		{
			//cout << "Is a BG process" << endl;
			int pos = tparts[i].find("&");
			tparts[i].erase(pos, 1);
			//cout << "Actually executing: " << tparts[i] << endl;
			execute (tparts [i]);
		}
		else if(isEcho)
		{
			tparts[i] = stripString(tparts[i]);
			cout << tparts[i] << endl;
			exit(0);
		}
		else
		{
			//cout << "About to execute: " << tparts[i] << endl;

                	execute (tparts [i]); // this is where you execute
		}
            }else{
		if(!isBG)
		{
                	waitpid(pid, 0, 0);            // wait for the child process
		}
		else
		{
			pids.push_back(pid); // push back pid into vector of them
			waitpid(pid, 0, WNOHANG);
			//cout << "Background" << endl;
		}
				// then do other redirects
		dup2(fd[0], 0);
		// TEMP here

		close(fd[1]);
				
		/*if(i == tparts.size()-2) // second to last time
			dup2(origOut, 1);
		if(i == tparts.size()-1)
		{
			dup2(origOut, 1);
			close(origOut);
			dup2(origIn, 0);
			close(origIn);*/

		}
            }
	    // After for loop
	    dup2(origOut, 1);
	    close(origOut);
	    dup2(origIn, 0);
	    close(origIn);

	    for(int i = 0; i < pids.size(); ++i)
	    {
		    int temp = waitpid(pids[i], 0, WNOHANG);
		    if(temp > 0) // process was finished remove it from vector
		    {
			    pidNames.erase(pidNames.begin()+i);
			    pids.erase(pids.begin()+i);
		    }
	    }
        }
	

    
}


