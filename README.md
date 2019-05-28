

# 5G-simulator
### _an open source framework to simulate LTE and 5G networks_
---
##### Table of Contents:
1. Getting 5G-simulator
2. Compiling 5G-simulator
3. Running 5G-simulator
4. How to do print debugging and logging
---
##### 1. Getting 5G-simulator
5G-simulator is available via Git at https://bitbucket.org/telematicslab/5g-simulator
To obtain 5G-simulator enter into the your prefered folder and write the following syntax:

    $ git clone https://bitbucket.org/telematicslab/5g-simulator.git
To synchronize the project repository with the local copy, you can run the pull sub-command. The syntax is as follows:

    $ git pull
##### 2. Compiling 5G-simulator
On recent Linux systems, first you need to install the armadillo library:

	$  sudo apt-get install libarmadillo-dev
Then you can build 5G-simulator with the following command:

	$ make
To clear the project, you can use the following command:

	$ make clean
##### 3. Running 5G-simulator
In this release several scenarios have been developed. To run a simple simulation, you can use the following command:

	$ ./5G-simulator Simple
For more details about available scenarios, use

	$ ./5G-simulator -h
##### 4. How to do print debugging and logging
5G-simulator leverages macros and environment variables to allow print debugging and logging without compiling the code each time.
The code must be placed between these macros:  

    DEBUG_LOG_START_1(LTE_SIM_FOO_VAR)
    ...
    //debug code
    ...
    DEBUG_LOG_END
 and each time you want to run that piece of code you just have to create the related environment variable, using the following command:

	$ export LTE_SIM_FOO_VAR=1

This operation can be undone by deleting the same environment variable:

	$ unset LTE_SIM_FOO_VAR

 5G-simulator works also with multiple environment variables at the same time (up to 4). For example:
 
    DEBUG_LOG_START_2(LTE_SIM_FOO_VAR, LTE_SIM_FOO_VAR_2)
    ...
    //debug code
    ...
    DEBUG_LOG_END
    
tells the compiler to run the code only if either LTE_SIM_FOO_VAR or LTE_SIM_FOO_VAR_2 have been defined.

Several instances are already present in the source files.

---
(c) 2018 - TELEMATICS LAB - Politecnico di Bari

### _Giuseppe Piro_
###### _Software manager and main developer_
giuseppe.piro@poliba.it

### _Alessandro Grassi_
###### _Developer_
alessandro.grassi@poliba.it

### _Sergio Martiradonna_
###### _Developer_
sergio.martiradonna@poliba.it

### _Gennaro Boggia_
###### _Project Supervisor_
gennaro.boggia@poliba.it