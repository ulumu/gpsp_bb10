/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>

#ifdef __QNXNTO__
#include <sys/slog.h>
#include <sys/slogcodes.h>

extern FILE *fLog;

#undef SLOG
#define SLOG(fmt, ...)   \
    if (fLog) {            \
       fprintf(fLog, "[%s][%s:%d]:"fmt"\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }

/*
    }  else  {             \
       slogf(_SLOG_SETCODE(_SLOGC_TEST+328, 0), _SLOG_DEBUG1, "[%s][%s:%d]:"fmt, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    }

*/

#ifdef __DEBUG__
#define DBGLOG(fmt, ...) slogf(_SLOG_SETCODE(_SLOGC_TEST+328, 0), _SLOG_DEBUG1, "[%s][%s:%d]:"fmt, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DBGLOG(fmt, ...)
#endif

#else
#define SLOG(fmt, ...) printf("[%s][%s:%d]:"fmt"\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#endif  //__QNXNTO__

#endif  //_LOGGING_H_
