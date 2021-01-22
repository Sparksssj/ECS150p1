# Report of project 1

## Overall idea of designing

After reading the the prompt,We roughly classified some different group of
things we need to implement.

1. We need to parse the use rinput command so we can handle it.
2. We need to differenciate the different kinds of commands so we can use 
multiple ways to handle them.
3. We need to handle pipes.
4. We need to do the extra feature things.
5. We need to handle errors if there are some, so the program will never crash
and give back sufficient error message to the user.

Listing out these ideas, we decided to implement them step by step.

### Parsing the user input

#### Initial try

When we decided to parse the user input, initially, we just tried to divide the 
commands by white spaces, remove the extra spaces, then store them back in the
command.

However, there are several problems to be fixed if we only do the initial try.
1. We changed the original command char* so that for the next time we want the 
initial command, we don't have it.
2. We cannot just parsing the command regardless the pipes, because the commands
between each two pipe signs are complete. In other words, we need to handle 
pipes first, and then we can parse the command between them.

#### Improvements

1. We used strdup to duplicate a command. The original one will never be 
modified so that we can use it at any time.
2. We handle pipes before we try to parse the command

### Handle different kinds of commands

From the prompt, we found there are many kinds of commands to be handled.
1. Non-fork commands like pwd,cd ...
2. Function that could be directly execute after fork.
3. Redirection
4. Commands with pipe.

#### Initial try

When tried to execute the first three kinds of command, and handle pipe later.
However, after completing those three ones, we found it is impossibe to 
seperately handle pipe because they are in fact closely related.

#### Improvements

As the parsing part, we restructed the code. First, we detect whether there is 
any pipe in the command. If not, just do what we did in the initial try. If
there is any pipe, we used recursion to handle the multiple blocks of commands.

### Implement the extra features

From the prompt, we decide to create an array to store the variables and use 
them when they are needed.

#### Initial try

We tried to add some functions in the regular command handling part.
Specifically, we create an array first, and every time we find the command is
"set", we will store the value in the array.

However, it doesn't work initially. Even we are 100% sure that the value is 
correctly stored into the array, it will still not work in the next command.

#### Improvements

Finally, we found the problem is we create the values array within the function,
which is a forked part of the program. As a result, for the next time we run a 
command, the forked program has exited. To solve the problem, we created a 
global variable so that we can directly modify it in any function, and we don't
need to worry it will disappear.


## Debug process


