
# ylog
A Simple, Lightweight, and Single-File Logging Library in C99
## Usage
Copy the **[ylog.h](ylog.h)**  file into your project. Include it among your source files.
## Example:
```c
#include "ylog.h"

void printlog(LogEvent *ev) {
	char *formatstr;
	YlogDefaultFormatString(ev, formatstr);
	printf("%s\n", formatstr);
}

int main() {
	YlogAddTrace(printlog);
	YlogTrace("Hello %s", "world!");
}
```
stdout:
```
[2024-12-29 21:45:55] TRACE - Hello world!
```
## Library
The library provides 16 function-like macros for logging:
```c
// Fire logging event
YlogTrace(const char *format, ...)
YlogDebug(const char *format, ...)
YlogAny(const char *format, ...)
...

// Add logging event callback
YlogAddTrace(void *func)
YlogAddInfo(void *func)
YlogAddAny(void *func)
...
```
Each function takes a printf format string followed by additional arguments:
```c
YlogTrace("Hello %s", "world!")
```
By this, a new log event gets created, executed and callbacks' functions get invoked. 
Each callback passes a parameter of LogEvent structure which is essentially:
```c
struct LogEvent {
	enum YLOGTYPE type;
	time_t time;
	char *msg;
}; 
```
There is a macro that constructs this LogEvent into a nice, clean string form:
```c
YlogDefaultFormatString(LogEvent *ev, char *result)
# Format:     [DATE     TIME]    TYPE  -   MESSAGE 
# Example: [2024-12-29 21:45:55] TRACE - Hello world!
```
## Thread-Safe
The library is also thread-safe if the **_YLOG_THREAD_SAFE** macro is defined.
If the macro is defined, then **Ylog_Threading_Init** function should be called before any functions called.
At the end **Ylog_Threading_Cleanup** function should be called.
```c
#define _YLOG_THREAD_SAFE
#include "ylog.h"

void printlog(LogEvent *ev);

int main() {
	Ylog_Threading_Init();
	YlogAddTrace(printlog);
	YlogTrace("Hello %s", "world!");
	Ylog_Threading_Cleanup();
}
```
## Cross-Platform
The library's thread-safe features are cross platform between windows and unix based operating systems.
```c
#ifdef _WIN32
	#include <windows.h>    
#else
	#include <pthread.h>
#endif
```
## Functions
#### int YlogAddCallback(LogCallback *cb)
Adds a **LogCallback pointer** into the callback list. 
Returns **YLOG_SUCCESS** on success. 
#### LogCallback *YlogCreateCallback(enum YLOGTYPE type, YLOG_CALLBACK_FUNCTION func)
Creates a new **LogCallback** structure. As the second parameter, the function is expecting a function pointer that accepts a **LogEvent pointer** as its parameter.
Returns the pointer of the created **LogCallback** structure.
#### void YlogExecuteEvent(LogEvent *ev)
Executes a **LogEvent**.
#### LogEvent *YlogCreateEvent(enum YLOGTYPE type, char *format, ...)
Creates a new **LogCallback** structure. The second parameter is the format and after the second, there are arguments for the format, just like printf.
Returns the pointer of the created **LogEvent** structure.
#### char *YlogEnumToString(enum YLOGTYPE type)
Converts **YLOGTYPE** enum value into its corresponding string value defined.
Returns the string value corresponding.
#### void Ylog_Threading_Init()
Initializes the library as **thread-safe**
#### void Ylog_Threading_Cleanup()
Cleans the library up from its **thread-safe** features
