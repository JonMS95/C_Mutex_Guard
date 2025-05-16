# C_Mutex_Guard: a POSIX thread mutexes handling library 🔒
This project aims to provide a useful tool to handle POSIX thread MutExes as well as some mechanisms to retrieve debugging information.


## Table of contents 🗂️
* [**Introduction** 📑](#introduction)
* [**Features** 🌟](#features)
* [**Prerequisites** 🧱](#prerequisites)
* [**Installation instructions** 📓](#installation-instructions)
  * [**Download and compile** ⚙️](#download-and-compile)
  * [**Compile and run test** 🧪](#compile-and-run-test)
* [**Usage** 🖱️](#usage)
* [**To do** ☑️](#to-do)
* [**Related documents** 🗄️](#related-documents)


## Introduction <a id="introduction"></a> 📑
Building this project has involved a deep understanding of:
* C/C++ programming
* Bash scripting
* Programming with POSIX threads
* Tests
* Linux OS operation

and the list goes on and on.

The project itself is nothing but a library that wraps POSIX threads mutexes, by making them safer in many aspects. The library has been built around a custom struct (**MTX_GRD**) in which useful information about where and who locked the mutex in question (apart from the mutex itself and its attributes) is found.
On to of that, errors can be retrieved, so it's especially helpful in cases in which mutex lock errors happen (such as deadlocks).
By now, it has been designed for it to be run on Linux distros (such as [Ubuntu](https://ubuntu.com/)). The resulting library is a *.so* file alongside a C language header (*.h*) file.


## Features <a id="features"></a> 🌟
*Structure type definitions: these are the core of the library. The main structure is called MTX_GRD. Its definition is shown below:

```C
typedef struct C_MUTEX_GUARD_ALIGNED
{
    pthread_mutex_t         mutex;
    pthread_mutexattr_t     mutex_attr;
    MTX_GRD_ACQ_LOCATION    mutex_acq_location;
    unsigned long long      lock_counter;
    void*                   additional_data;
} MTX_GRD;
```

However, it should be noted that instances of MTX_GRD should not be directly modified. Instead, API functions should be used (having included the struct definition in the API instead of making it opaque was just a performance-related decision). 
Some other definitions (for both strutures and enum types) have been included: *MTX_GRD_ACQ_LOCATION*, *MTX_GRD_LOCK_TYPES* and *MTX_GRD_VERBOSITY_LEVEL*. Again, those are not meant to be directly modified.

* Functions: the library includes functions that can be divided into the following categories:
    * Create/Init/Destroy: they create, initialize and/or destroy mutex structure and mutex attributes (_MutexGuardInit_, _MutexGuardLock_, _MutexGuardUnlock_ or _MutexGuardDestroy_ among others).
    * Lock/Unlock: macros to lock or unlock the mutex in question.
    * Error retrieval functions: same as strerr functions (included in string.h), some of the functions retrieve the error that caused any of the functions to fail.
    * Cleanup functions: these are not meant to be directly called by the user. Instead, they are meant to be used by the macros. Their purpose is to undo a target task (such as unlocking a mutex if it was locked beforehand, or destroy a mutex if it was created within the same scope).
    * _MutexGuardSetPrintStatus_: allows the user to modify the number of messages that will be automatically displayed by the module. Using it alongside _MTX_GRD_VERBOSITY_LEVEL_ is strongly recommended.
    * _MutexGuardGetFuncRetAddr_: different from cleanup functions, this one can be directly called but it's not its main purpose. It retrieves the address from where the function in question was called. It's meant to be helpful when calling _MutexGuardLock_ function.

* Macros: the library includes macros that make the usage of the functions in the library easier. They can be divided into the following categories:
    * Non-scoped: they behave as usual, it's to say, their behaviour is just a matter of how or where were the called.
    * Scoped: They will undo the task in question as soon as the scope in which they were called is exited. For instance, if MTX_GRD_TRY_LOCK macro is called within a function and a mutex has been locked successfully, the mutex in question will be automatically released as soon as the thread exits the function's scope.

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
| [CUnit][cunit-link]          | Unit tests                              |2.1-3           |

[gcc-link]:        https://gcc.gnu.org/
[bash-link]:       https://www.gnu.org/software/bash/
[make-link]:       https://www.gnu.org/software/make/
[git-link]:        https://git-scm.com/
[xmlstarlet-link]: https://xmlstar.sourceforge.net/
[cunit-link]:      https://cunit.sourceforge.net/

Except for Make and Bash, the latest version of each of the remaining dependencies will be installed automatically if they have not been found beforehand. 

In any case, installing **_Xmlstarlet_** before executing any of the commands below is strongly recommended. Otherwise, it can lead to an error since make file
contains some calls to it at the top. If that happens, just repeat the process (Xmlstarlet would have been already installed).

On top of the ones listed above, there are some *JMS* dependencies (libraries that were also made by myself) that are required for both the library and the test executable to be built,
(although these are managed by the library itself, so no need to download them manually). The required version for each of them is specified by the [config.xml](config.xml) file.

| Dependency                                                              | Purpose                                  |
| :---------------------------------------------------------------------- | :--------------------------------------- |
| [C_Common_shell_files](https://github.com/JonMS95/C_Common_shell_files) | Process [configuration file](config.xml) |
| [C_Severity_Log](https://github.com/JonMS95/C_Severity_Log)             | Show logs                                |


## Installation instructions <a id="installation-instructions"></a> 📓
### Download and compile <a id="download-and-compile"></a> ⚙️
1. In order to download the repo, just clone it from GitHub to your choice path by using the [link](https://github.com/JonMS95/C_Mutex_Guard) to the project.

```bash
cd /path/to/repos
git clone https://github.com/JonMS95/C_Mutex_Guard
```

2. Then navigate to the directory in which the repo has been downloaded, and set execution permissions to every file just in case they have not been sent beforehand.

```bash
cd /path/to/repos/C_Mutex_Guard

find . -type f -exec chmod u+x {} +
```

3. For the library to be built (i.e., clean, download dependencies and compile), just type the following:

```bash
make
```

The result of the line above will be a new API directory (which will match the used version). Within it, a *.h* and a *.so* file will be found.
- **/path/to/repos/C_Mutex_Guard/API**
  - **vM_m**
    - **lib**
      - **_libMutexGuard.so.M.m_**
    - **Header_files**
      - **_MutexGuard_api.h_**

Where **_M_** and **_m_** stand for the major and minor version numbers.
**_MutexGuard_api.h_** could also be found in **_/path/to/repos/C_Mutex_Guard/src/MutexGuard_api.h_** although it may differ depending on the version.


### Compile and run test <a id="compile-and-run-test"></a> 🧪
For the test executable file to be compiled and executed, use:

```bash
make test
```

Again, the one below is the path to the generated executable file:
- **/path/to/repos/C_Mutex_Guard/test**
  - **exe**
      - **_main_**
  - src
  - deps


## Usage <a id="usage"></a> 🖱️
See Doxygen comments placed over every macro, function definition and struct type definition in the API header file ([api-file](src/MutexGuard_api.h)).
Tests source files (especially the ones found within [TestDemos.c](test/src/TestDemos.c) can provide deep insight as well).


## To do <a id="to-do"></a> ☑️
Nothing to be done by now.


## Related Documents <a id="related-documents"></a> 🗄️
* [LICENSE](LICENSE)
* [CONTRIBUTING.md](docs/CONTRIBUTING.md)
* [CHANGELOG.md](docs/CHANGELOG.md)

