# SSHELL: A Simple Shell Called Sshell

## Summary

This program, 'sshell', is a command-line intepreter supports redirection and 
pipe.

```
$ ./sshell 
```

The executable sshell does not take any input arguments. Within the program, it 
can execute user-input commands and print the complete message to standard 
error.

## Implementation

The implementation of this program follows three distinct steps:
1. Parsing the input command line. 
2. Handle and execute the command.
3. Display the complete message to standard error.

Note: Variables names are **bold**. Function names are _italic_.

### Parsing the Command Line

#### Separated Command Line by Pipe Character
Input command line is separeted by pipe character '|' and stored in an array of 
string named **pip**, where each string is a command with spaces removed 
(leading spaces and trailing spaces removed, spaces removed will mean same thing
for later use in this document). If no pipe character detected, **pip[0]** 
stores the original command line with spaces removed. 

#### Separated Each Command by Redirection Character
Then information about pipe will be passed as arguments into the function 
_handleparingerrors_, which returns an interger indicated the type of paring 
error found. This function iterates through each command i.e. **pip[i]**, 
which will be separated by redirection character '>' and stored in an array of
string named **rdc** with spaces removed. Thus, **rdc[0]** is the command 
before '>', **rdc[1]** is ">', and **rdc[2]** is the filename after '>'. If no
redirection character detected, **rdc[0]** will be the original command with 
spaces removed.
 
#### Specific Parsing Errors
If no redirection character detected, then we only need to check if number of 
arguments input is less than sixteen and if the variable after '$' is between
'a' and 'z'. If redirection character is detected, the following will be checked 
in order: For **rdc[0]**, is it empty, is variable after '$' between 'a' and 
'z', is number of arguments less than sixteen; for **rdc[1]**, is this command 
the last command; for **rdc[2]**, is it empty, do have write permission for this
file. By following this order, the left-most paring error will be detected 
first. If any error is detected, the integer value correspoding to it will be 
returned, and if no error detected, number 0 will be returned. 

#### Handle Return Value of _Handleparsingerror_ Function 
If the return value of _handlepasingerror_ function is not 0, error message 
associated with return value will be printed to standerror, and user will be
asked for next command. If the return value is 0, move forward to the next step
handle and execute the command.

### Handle and execute the command

Three different conditions for this step.


#### First Condition
If the command is cd, pwd, or set. Then the program will step into the 
_nonforkfunc_ function since we don't need to fork a child process to handle 
these three commands.

##### Separation of Command into Arguments
Command will be separated as arguments and stored into an array of string named
**parsedcmd**. Where **parsedcmd[0]** is the command itself, and 
**parsedcmd[1]** to **parsedcmd[-1]** are arguments. 

##### Specific Way of Handling pwd,cd, set
Then, cd and pwd are handled by build-in functions _chdir()_ and 
_get_current_dir_name()_. For set, a global variable of an array of string named 
**storedvarialbe** is created. It has length of 26, where **storedvariable[0]** 
stores what is assigned to 'a' and **storedvariable[25]** stores what is asigned
to 'z'. Since **storedvariables** is a global variable, it will be initialized 
with 0s. Thus, when handling later commands, if '$' is detected, "$character" 
will be replaced with the value in **storedvariables[corresponding index]** if 
the value is not 0, otherwise replaced with empty string "". 

#### Second Condition
If no pipe character detected, and it's not the first condition. Then 
the program will step into the _onepipe_ function (here one pipe means one
command, so no pipes). 

##### Explanation of _Onepipe_ 
In this case, fork will be called once to handle the command. If the current
process is parent, it will wait for the child and get child's exit status. 
If the current process is child, it will call the void function 
_handlearguments_, which takes the command and information about redirection 
as arguments. 

##### Explanation of _Handlearguments_ 
Within this function, command will be separated into arguments, and then 
function to execute the process will be called. If redirection character 
is detected, then output will be redirected to the file by using _dup2_ 
before executing the process.

#### Third Condition
If pipe character is detected. Then the program will step into _multpipe_
function. 

##### Explanation of _Multpipe_
_Multpipe_ is a recursive function. It will initially create a pipe and call 
fork. If the current process is parent, it waits for its child process and 
get child's exit status. Then, if it's the first process along pipes, it 
connects its standard input to the current pipe and calls _handlearguments_ 
function. If it's one of the middle processes, it connects its standard input
to the previous pipe and connects its standard output to the current pipe and 
calls _handlearguments_ function. If the current process is child, it connects 
its standard output to the current pipe and calls _handlearguments_ function if
this is the last process along pipes, otherwise, calls _multpipe_ again. 
_Handlearguments_ function will do the same thing as in the second condition.

##### Order of Execution of Parent and Children Processes
By implementing _multpipe_ function in this way, the child process in the last 
called _multpipe_ will be handled first. The parent process in the last called
_multpipe_ will be handled second, and then the parent process in the second
last called _multpipe_ and so on. Finally the parent process in the first called 
_multpipe_ will be handled the last, and no child process executes the program 
except the one in the last called _multpipe_. 

##### Order of Execution of Processes in Command Line
Here, the child process in the last called _multpipe_ executes the first process 
along pipes, the parent process in the last and middle called _multpipe_ 
executes the middle processes along pipes, and the parent process in the first 
called _multpipe_ executes the last process along pipes. 

##### A Clear Demonstration 
Therefore, _multpipe_ function will handle commands like below, 
process1 -> pipe1 -> process2 -> pipe2 -> ... -> pipelast -> processlast -> 
standard output. 

### Display the Completed Message

#### Print Out Original Command Line
The initial user-entered command line will be duplicated and passed for parsing
and handling. Thus, the original command line will be printed as part of 
completed message. Another part of completed message is the exit status of
each command.

#### Print Out Exit Status
For condition one and two in the previous step, an integer value will be 
returned to the main function and printed out as exit status. For condition 
three, an integer array named **message** is created in the main function, and 
passed as an argument into _multpipe_ function. Then **message[i]** contains 
the exit status of the (i-1)th command. 

## Extensibility

Note: Macro names are **bold**. Enum names are _italic_.

### Macro
Maximum number of characters a command line can have is defined in macro as 
**CMDLINE_MAX**. Exit status of a program is either 0, 256, or 512. In order to
make it 0, 1, or 2, it has to be divided by 256, and this value is defined in
macro as **ExitStatusDevision**. Maximum number of arguments is defined in 
macro as **MaximunArguments**. ASCII value of lowercase a is defined in macro
as **lowercaseA**, and ASCII value of lowercase z is defined in macro as 
**lowercaseZ**.

### Enum
Return values of parsing errors is defined in enum _ParsingError_.














 




