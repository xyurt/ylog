/*
 * Copyright (c) 2024 xyurt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _YLOG_H 
#define _YLOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#ifdef _YLOG_THREAD_SAFE
#ifdef _WIN32
#include <windows.h>    
#else
#include <pthread.h>
#endif
#endif

#define YLOG_FAILURE 0
#define YLOG_SUCCESS 1

#define YlogAdd(type, func) YlogAddCallback(YlogCreateCallback(type, func)
#define Ylog(type, format, ...) YlogExecuteEvent(YlogCreateEvent(type, format, ##__VA_ARGS__))

#define YlogAddTrace(func) YlogAddCallback(YlogCreateCallback(YLOGTRACE, func))
#define YlogAddDebug(func) YlogAddCallback(YlogCreateCallback(YLOGDEBUG, func))
#define YlogAddInfo(func) YlogAddCallback(YlogCreateCallback(YLOGINFO, func))
#define YlogAddWarn(func) YlogAddCallback(YlogCreateCallback(YLOGWARN, func))
#define YlogAddError(func) YlogAddCallback(YlogCreateCallback(ERROR, func))
#define YlogAddFatal(func) YlogAddCallback(YlogCreateCallback(YLOGFATAL, func))
#define YlogAddAny(func) YlogAddCallback(YlogCreateCallback(YLOGANY, func))

#define YlogTrace(format, ...) YlogExecuteEvent(YlogCreateEvent(YLOGTRACE, format, ##__VA_ARGS__))
#define YlogDebug(format, ...) YlogExecuteEvent(YlogCreateEvent(YLOGDEBUG, format, ##__VA_ARGS__))
#define YlogInfo(format, ...) YlogExecuteEvent(YlogCreateEvent(YLOGINFO, format, ##__VA_ARGS__))
#define YlogWarn(format, ...) YlogExecuteEvent(YlogCreateEvent(YLOGWARN, format, ##__VA_ARGS__))
#define YlogError(format, ...) YlogExecuteEvent(YlogCreateEvent(ERROR, format, ##__VA_ARGS__))
#define YlogFatal(format, ...) YlogExecuteEvent(YlogCreateEvent(YLOGFATAL, format, ##__VA_ARGS__))
#define YlogAny(format, ...) YlogExecuteEvent(YlogCreateEvent(YLOGANY, format, ##__VA_ARGS__))

#ifdef _MSC_VER 
#define YlogDefaultFormatString(ev, out) { \
    char formatted_time[80]; \
    YlogTimeConvert((ev)->time, 80, formatted_time); \
    int len = snprintf(NULL, 0, "[%s] %s - %s", formatted_time, YlogEnumToString((ev)->type), (ev)->msg) + 1; \
    (out) = malloc(len); \
    if ((out)) { \
        snprintf((out), len, "[%s] %s - %s", formatted_time, YlogEnumToString((ev)->type), (ev)->msg); \
    } \
}

#define YlogTimeConvert(ttime, size, result) { \
	struct tm time_info; \
	localtime_s(&time_info, &(ttime)); \
	strftime(result, size, "%Y-%m-%d %H:%M:%S", &time_info); \
}
#endif

struct LogEvent {
	enum YLOGTYPE type;
	time_t time;
	char *msg;
};
typedef struct LogEvent LogEvent;

typedef void (*YLOG_CALLBACK_FUNCTION)(LogEvent *event);

struct LogCallback {
	enum YLOGTYPE type;
	YLOG_CALLBACK_FUNCTION func;
};
typedef struct LogCallback LogCallback;

#ifdef _YLOG_THREAD_SAFE
#ifdef _WIN32
CRITICAL_SECTION YLOG_THREADING_CS;
#else
pthread_mutex_t YLOG_THREADING_CS;
#endif
#endif

int YLOG_CALLBACK_COUNT = 0;
LogCallback **YLOG_LOG_CALLBACKS = NULL;

enum YLOGTYPE { YLOGTRACE, YLOGDEBUG, YLOGINFO, YLOGWARN, YLOGERROR, YLOGFATAL, YLOGANY };
char *YLOG_LOGTYPE_STRINGS[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "ANY" };

void Ylog_Lock_Thread(void);
void Ylog_Unlock_Thread(void);

int Ylog_Log_Callback_Increase(void);
int Ylog_Alloc_Log_Callback_List(void);
int Ylog_Realloc_Log_Callback_List(void);
void Ylog_Free_Log_Callback_List(void);

char *YlogEnumToString(enum YLOGTYPE type);

LogEvent *YlogCreateEvent(enum YLOGTYPE type, char *format, ...);
void YlogExecuteEvent(LogEvent *ev);

LogCallback *YlogCreateCallback(enum YLOGTYPE type, YLOG_CALLBACK_FUNCTION func);
int YlogAddCallback(LogCallback *cb);

// Creates a new log event
LogEvent *YlogCreateEvent(enum YLOGTYPE type, char *format, ...) {
	LogEvent *ev = (LogEvent *)malloc(sizeof(LogEvent));
	if (!ev) return NULL;
	ev->type = type;
	time(&ev->time);

	va_list args; va_start(args, format);

	int requiredSize = vsnprintf(NULL, 0, format, args);
	if (requiredSize < 0) { va_end(args); return ev; }
	ev->msg = (char *)malloc((requiredSize + 1) * sizeof(char));
	if (!ev->msg) { va_end(args); return ev; }
	vsnprintf(ev->msg, requiredSize + 1, format, args);

	va_end(args);
	return ev;
}

// Creates a new callback with given parameters
LogCallback *YlogCreateCallback(enum YLOGTYPE type, YLOG_CALLBACK_FUNCTION func) {
	if (!func) return NULL;
	LogCallback *cb = (LogCallback *)malloc(sizeof(LogCallback));
	if (cb) { cb->type = type, cb->func = func; }
	return cb;
}

// Adds the given callback into the callback list
int YlogAddCallback(LogCallback *cb) {
	if (!cb || !cb->func) return YLOG_FAILURE;

	if (!Ylog_Log_Callback_Increase()) return YLOG_FAILURE;

	Ylog_Lock_Thread();
	YLOG_LOG_CALLBACKS[YLOG_CALLBACK_COUNT - 1] = cb;
	Ylog_Unlock_Thread();
	return YLOG_SUCCESS;
}

// Execute a log event on every log callbacks
void YlogExecuteEvent(LogEvent *ev) {
	if (!ev) return;
	Ylog_Lock_Thread();
	for (int i = 0; i < YLOG_CALLBACK_COUNT; i++) {
		LogCallback *cb = YLOG_LOG_CALLBACKS[i];
		if (!(ev->type == cb->type || ev->type == YLOGANY || cb->type == YLOGANY)) continue;
		if (cb->func) cb->func(ev);
	}
	free(ev);
	Ylog_Unlock_Thread();
}

// Converts enum type to string
char *YlogEnumToString(enum YLOGTYPE type) {
	if (type >= YLOGTRACE && type <= YLOGANY) {
		return YLOG_LOGTYPE_STRINGS[type];
	}
	return "UNKNOWN";
}

int Ylog_Log_Callback_Increase(void) {
	Ylog_Lock_Thread();
	if (YLOG_CALLBACK_COUNT < 0) YLOG_CALLBACK_COUNT = 0;
	YLOG_CALLBACK_COUNT++;
	Ylog_Unlock_Thread();
	return Ylog_Realloc_Log_Callback_List();
}

int Ylog_Realloc_Log_Callback_List(void) {
	if (YLOG_CALLBACK_COUNT < 1) { Ylog_Free_Log_Callback_List(); return YLOG_SUCCESS; }
	if (!YLOG_LOG_CALLBACKS) { return Ylog_Alloc_Log_Callback_List(); }

	Ylog_Lock_Thread();
	LogCallback **YLOG_LOG_CALLBACKS_temp = (LogCallback **)realloc(YLOG_LOG_CALLBACKS, YLOG_CALLBACK_COUNT * sizeof(LogCallback *));
	if (!YLOG_LOG_CALLBACKS_temp) { Ylog_Unlock_Thread(); return YLOG_FAILURE; }

	YLOG_LOG_CALLBACKS = YLOG_LOG_CALLBACKS_temp;

	Ylog_Unlock_Thread();
	return YLOG_SUCCESS;
}

int Ylog_Alloc_Log_Callback_List(void) {
	if (YLOG_LOG_CALLBACKS) { Ylog_Free_Log_Callback_List(); }

	Ylog_Lock_Thread();
	if (YLOG_CALLBACK_COUNT < 1) { Ylog_Unlock_Thread(); return YLOG_SUCCESS; }

	YLOG_LOG_CALLBACKS = (LogCallback **)malloc(YLOG_CALLBACK_COUNT * sizeof(LogCallback *));
	if (YLOG_LOG_CALLBACKS == NULL) { Ylog_Unlock_Thread(); return YLOG_FAILURE; }

	Ylog_Unlock_Thread();
	return YLOG_SUCCESS;
}

void Ylog_Free_Log_Callback_List(void) {
	Ylog_Lock_Thread();
	if (!YLOG_LOG_CALLBACKS) { Ylog_Unlock_Thread(); return; }
	for (int i = 0; i < YLOG_CALLBACK_COUNT; i++) {
		LogCallback *cb = YLOG_LOG_CALLBACKS[i];
		if (!cb) continue;
		free(cb);
	}
	free(YLOG_LOG_CALLBACKS);
	YLOG_CALLBACK_COUNT = 0;
	YLOG_LOG_CALLBACKS = NULL;
	Ylog_Unlock_Thread();
	return;
}

int YLOG_THREAD_SAFE_HAS_STARTED;

void Ylog_Lock_Thread(void) {
#ifdef _YLOG_THREAD_SAFE
	if (!YLOG_THREAD_SAFE_HAS_STARTED) return;
#ifdef _WIN32
	EnterCriticalSection(&YLOG_THREADING_CS);
#else
	pthread_mutex_lock(&YLOG_THREADING_CS);
#endif
#endif
}

void Ylog_Unlock_Thread(void) {
#ifdef _YLOG_THREAD_SAFE
	if (!YLOG_THREAD_SAFE_HAS_STARTED) return;
#ifdef _WIN32
	LeaveCriticalSection(&YLOG_THREADING_CS);
#else
	pthread_mutex_unlock(&YLOG_THREADING_CS);
#endif
#endif
}

#ifdef _YLOG_THREAD_SAFE
void Ylog_Threading_Init() {
	YLOG_THREAD_SAFE_HAS_STARTED = 1;
#ifdef _WIN32
	InitializeCriticalSection(&YLOG_THREADING_CS);
#else
	pthread_mutex_init(&YLOG_THREADING_CS, NULL);
#endif
}

void Ylog_Threading_Cleanup() {
	YLOG_THREAD_SAFE_HAS_STARTED = 0;
#ifdef _WIN32
	DeleteCriticalSection(&YLOG_THREADING_CS);
#else
	pthread_mutex_destroy(&YLOG_THREADING_CS);
#endif
}
#endif // _YLOG_THREAD_SAFE
#endif //YLOG_H