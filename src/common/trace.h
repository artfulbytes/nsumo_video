#ifndef TRACE_H
#define TRACE_H

#define TRACE(fmt, ...) trace("%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#ifndef DISABLE_TRACE
void trace_init(void);
void trace(const char *format, ...);
#else
#define trace_init() ;
#define trace(fmt, ...) ;
#endif

#endif // TRACE_H
