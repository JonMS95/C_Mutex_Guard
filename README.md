# C_Mutex_Guard: a POSIX thread mutexes handling library 
The aim of this project is to build and learn about internet sockets, so it has been created for both educational and practical purposes.


## Table of contents 🗂️
* [**Introduction** 📑](#introduction)
* [**Features** 🌟](#features)
* [**Prerequisites** 🧱](#prerequisites)
* [**Installation instructions** 📓](#installation-instructions)
  * [**Download and compile** ⚙️](#download-and-compile)
  * [**Create certificate and private key** 🔐](#create-certificate-and-private-key)
  * [**Compile and run test** 🧪](#compile-and-run-test)
* [**Usage** 🖱️](#usage)
* [**To do** ☑️](#to-do)
* [**Related documents** 🗄️](#related-documents)


## Introduction <a id="introduction"></a> 📑
This library was started as a way for me to have a more practical approach to how TCP sockets work. I felt a bit overwhelmed at first by the large quantity of concepts I had to get a deeper understanding of.
However, as I found out the vast amount of practical applications it may have in real life, I started regarding it as a way more amusing and enriching activity for my professional career.

Building this project has involved a deep understanding of:
* C/C++ programming
* Bash scripting
* TCP/IP protocol stack
* Dependency management
* Parallel-running processes
* Tests
* Linux OS operation

and the list goes on and on.
Despite existing some aspects that need to be improved, it's a project I feel quite proud of. More of those (such as UDP support) will be coming soon.
By now, it has been designed for it to be run on Linux distros (such as [Ubuntu](https://ubuntu.com/)). The resulting library is a *.so* file alongside a C language header (*.h*) file.


## Features <a id="features"></a> 🌟
The resulting library comes with many options that can be applied to it, such as setting timeouts, or adding a custom interaction function,
it's to say, an arbitrary function to describe how the socket should interact once the client has connected to it.

When it comes to security, TLS can be set, as well as the paths to the private key and server certificate, which should be created by the user itself
(these files are not provided with the current project for privacy concerns).

In order to get some knowledge about how to use the library alongside its options, go to [Usage](#usage).


## Prerequisites <a id="prerequisites"></a> 🧱
By now, the application has only been tested in POSIX-compliant Linux distros. In these, many of the dependencies below may already come installed in the OS.
In the following list, the minimum versions required (if any) by the library are listed.

| Dependency                   | Purpose                                 | Minimum version |
| :--------------------------- | :-------------------------------------- |:-------------: |
| [gcc][gcc-link]              | Compile                                 |11.4            |
| [Bash][bash-link]            | Execute Bash/Shell scripts              |4.4             |
| [Make][make-link]            | Execute make file                       |4.1             |
| [Git][git-link]              | Download GitHub dependencies            |2.34.1          |
| [Xmlstarlet][xmlstarlet-link]| Parse [configuration file](config.xml)  |1.6.1           |
| [OpenSSL][openssl-link]      | Allow TLS                               |3.0.2           |

[gcc-link]:        https://gcc.gnu.org/
[bash-link]:       https://www.gnu.org/software/bash/
[make-link]:       https://www.gnu.org/software/make/
[git-link]:        https://git-scm.com/
[xmlstarlet-link]: https://xmlstar.sourceforge.net/
[openssl-link]:    https://www.openssl.org/

Except for Make, Bash and OpenSSL, the latest version of each of the remaining dependencies will be installed automatically if they have not been found beforehand. 

When it comes to OpenSSL, its version may be earlier than the required one. In that case, it should be upgraded to 3.0.2 version at least. [**Shell_files/update_openssl.sh**](Shell_files/update_openssl.sh) should do the work (it will install 3.0.2 version, if any other version is wanted, just modify the script line in which the target openssl tgz file is downloaded). If developer files (such as libraries and header files) cannot be found, the following couple of lines should sort it out (even when a suitable version of OpenSSl has already been installed):

```bash
sudo apt-get update
sudo apt-get install libssl-dev
```

In any case, installing **_Xmlstarlet_** before executing any of the commands below is strongly recommended. Otherwise, it can lead to error since make file
contains some calls to it at the top. If that happens, just repeat the process (Xmlstarlet would have been already installed).

On top of the ones listed above, there are some *JMS* dependencies (libraries that were also made by myself) that are required for both the library and the test executable to be built,
(although these are managed by the library itself, so no need to download them manually). The required version for each of them is specified by the [config.xml](config.xml) file.

| Dependency                                                              | Purpose                                  |
| :---------------------------------------------------------------------- | :--------------------------------------- |
| [C_Common_shell_files](https://github.com/JonMS95/C_Common_shell_files) | Process [configuration file](config.xml) |
| [C_Severity_Log](https://github.com/JonMS95/C_Severity_Log)             | Show logs                                |
| [C_Arg_Parse](https://github.com/JonMS95/C_Arg_Parse)                   | Parse command line arguments             |
| [C_Client_Socket](https://github.com/JonMS95/C_Client_Socket)           | Test alongside server socket             |


## Installation instructions <a id="installation-instructions"></a> 📓
### Download and compile <a id="download-and-compile"></a> ⚙️
1. In order to download the repo, just clone it from GitHub to your choice path by using the [link](https://github.com/JonMS95/C_Server_Socket) to the project.

```bash
cd /path/to/repos
git clone https://github.com/JonMS95/C_Server_Socket
```

**Note**: by default, the path to the repository should be found within a directory (found in Desktop) called "scripts" (it's to say, **_~/Desktop/scripts/C_**). In order to modify this, change the paths specified in [config.xml](config.xml). If dependencies are not found in the specified path, the path itself would be created, then dependencies will be compiled there. 

2. Then navigate to the directory in which the repo has been downloaded, and set execution permissions to every file just in case they have not been sent beforehand.

```bash
cd /path/to/repos/C_Server_Socket

find . -type f -exec chmod u+x {} +
```

3. For the library to be built (i.e., clean, download dependencies and compile), just type the following:

```bash
make
```

The result of the line above will be a new API directory (which will match the used version). Within it, a *.h* and a *.so* file will be found.
- **/path/to/repos/C_Server_Socket/API**
  - **vM_m**
    - **Dynamic_libraries**
      - **_libServerSocket.so.M.m_**
    - **Header_files**
      - **_ServerSocket_api.h_**

Where **_M_** and **_m_** stand for the major and minor version numbers.
**_ServerSocket_api.h_** could also be found in **_/path/to/repos/C_Server_Socket/Source_files/ServerSocket_api.h_** although it may differ depending on the version.

### Create certificate and private key <a id="create-certificate-and-private-key"></a> 🔐

If executing the default test is not wanted but one without any security requirements, then locally modify [Shell_files/test.sh](Shell_files/test.sh) or run a custom
command so that the executable does not use -s, -k and -c options, then go directly to the [test compilation stage](#compile-and-run-test). Otherwise, follow the
steps described below.



Steps to follow for the certificate and private key to be created using OpenSSL:

1. Generate private key:

   ```sh
   openssl genpkey -algorithm RSA -out private.key
   ```
   
2. (Optional) Set proper permissions to private key, so that only user is able to read or write it:

   ```sh
   chmod 600 private.key
   ```

3. Generate certificate (self-signed) based on previously created key:

   ```sh
   openssl req -new -key private.key -x509 -days 365 -out certificate.crt
   ```

4. (Optional) Read certificate content:
  
   ```sh
   openssl x509 -in certificate.crt -text
   ```


### Compile and run test <a id="compile-and-run-test"></a> 🧪
For the test executable file to be compiled and executed, use:

```bash
make test
```

Again, the one below is the path to the generated executable file:
- **/path/to/repos/C_Server_Socket/Tests**
  - **Executable_files**
      - **_main_**
  - Source_files
  - Dependency_files


## Usage <a id="usage"></a> 🖱️
The following is the main server socket function prototype as found in the **_header API file_** (_/path/to/repos/C_Server_Socket/API/vM_m/Header_files/ServerSocket_api.h_) or in the [repo file](Source_files/ServerSocket_api.h).

```c
C_SERVER_SOCKET_API int ServerSocketRun(int             server_port                                     ,
                                        int             max_conn_num                                    ,
                                        bool            concurrent                                      ,
                                        bool            non_blocking                                    ,
                                        bool            reuse_address                                   ,
                                        bool            reuse_port                                      ,
                                        unsigned long   rx_timeout_s                                    ,
                                        unsigned long   rx_timeout_us                                   ,
                                        unsigned long   tx_timeout_s                                    ,
                                        unsigned long   tx_timeout_us                                   ,
                                        bool            secure                                          ,
                                        const char*     cert_path                                       ,
                                        const char*     pkey_path                                       ,
                                        int             (*CustomSocketStateInteract)(int client_socket) );
```

The amount of input parameters can be a bit astonishing, but please keep in mind it has been designed this way to
provide maximum optionality so each developer can customize its usage as much as possible. Here is a list of the meaning of each input parameter:
* **server_port**: specifies the port the server is meant to listen to
* **max_conn_num**: maximum number of connections /clients
* **concurrent**: specifies whether or not could those connections be parallel
* **non_blocking**: tells whether or not is the socket non-blocking (not recommended)
* **reuse_address**: reuse address, does not hold the address after the socket is closed (strongly recommended)
* **reuse_port**: reuse port, does not hold the port after the socket is closed (strongly recommended)
* **rx_timeout_s**: receive timeout in seconds.
* **rx_timeout_us**: receive timeout in microseconds.
* **tx_timeout_s**: send timeout in seconds.
* **tx_timeout_us**: send timeout in microseconds.
* **secure**: enable secure communication (TLS).
* **cert_path**: path to the server certificate.
* **key_path**: path to the server private key.
* **CustomSocketStateInteract**: custom function to interact with the client once a connection has been established.

In exchange, the function returns:
* **0** if everything went OK, or exit sending **failure signal** if SIGINT signal handler could not be properly set

For reference, a proper API usage example has been provided on the [test source file](Tests/Source_files/main.c).
As this one uses [**C_Arg_Parse library**](https://github.com/JonMS95/C_Arg_Parse), input parameters can be provided by using command-line interface.
An example of CLI usage is provided in the [**Shell_files/test.sh**](Shell_files/test.sh) file.


## To do <a id="to-do"></a> ☑️
- [ ] Add UDP support
- [ ] Use threads instead of parallel processes (more handleable)


## Related Documents <a id="related-documents"></a> 🗄️
* [LICENSE](LICENSE)
* [CONTRIBUTING.md](Docs/CONTRIBUTING.md)
* [CHANGELOG.md](Docs/CHANGELOG.md)

