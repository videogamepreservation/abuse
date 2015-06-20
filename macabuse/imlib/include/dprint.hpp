#ifndef DPRINT_HPP_
#define DPRINT_HPP_

void set_dprinter(void (*stat_fun)(char *));       // called with debug info
void set_dgetter(void (*stat_fun)(char *, int));   // called mainly by lisp breaker
void dprintf(const char *format, ...);
void dgets(char *buf, int size);


#endif
