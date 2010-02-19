/* Utilities & standard top-level types
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "config.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <sstream>
using namespace std;

flo urnd(flo min, flo max) { return min + ((max-min)*rand())/RAND_MAX; }

/*****************************************************************************
 *  NOTIFICATION FUNCTIONS                                                   *
 *****************************************************************************/
void uerror (char* message, ...) {
  void **x = 0;
  va_list ap;
  va_start(ap, message);
  vprintf(message, ap);  printf("\n Aborting on fatal error\n"); fflush(stdout); 
  va_end(ap);
  //*x = (void*)1;
  exit(1);
}

void post_into(Strbuf *buf, char* pstring, ...) {
  int n;
  va_list ap;
  va_start(ap, pstring);
  buf->idx += vsprintf(&buf->data[buf->idx], pstring, ap);
  va_end(ap);
  fflush(stdout);
}

void debug(char* dstring, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, dstring);
  vsprintf(buf, dstring, ap);
  va_end(ap);
  fputs(buf, stderr);
  fflush(stderr);
}

void post(char* pstring, ...) {
  va_list ap;
  va_start(ap, pstring);
  vprintf(pstring, ap);
  va_end(ap);
  fflush(stdout);
}

/*****************************************************************************
 *  MEMORY MANAGEMENT                                                        *
 *****************************************************************************/
#define GC_malloc malloc
#define GC_free   free

void *MALLOC(size_t size) { return malloc(size); }

void FREE(void *ptr_) {
  void **ptr = (void**)ptr_;
  if (*ptr == NULL) uerror("TRYING TO FREE ALREADY FREED MEMORY\n");
  GC_free(*ptr);
  *ptr = NULL;
}

/*****************************************************************************
 *  COMMAND LINE ARGUMENT SUPPORT                                            *
 *****************************************************************************/
void Args::remove(int i) {
  argc--; // shrink the size of the list
  for(;i<argc;i++) { argv[i]=argv[i+1]; } // shuffle args backward
}

// ARG_SAFE is used to check if two different things modules request the
// same argument
#define ARG_SAFE TRUE
#define MAX_SWITCHES 200
static int num_switch_tests = 0;
static char switch_rec[MAX_SWITCHES][64]; // remember a set of <64-byte args
BOOL Args::extract_switch(char *sw) { return extract_switch(sw,TRUE); }
BOOL Args::extract_switch(char *sw, BOOL warn) {
  if(ARG_SAFE && warn) { // warns the user if a switch is overloaded
    for(int i=0;i<num_switch_tests;i++) {
      if(strcmp(sw,switch_rec[i])==0) {
	debug("WARNING: Switch '%s' used more than once.\n",sw); break;
      }
    }
    if(num_switch_tests<MAX_SWITCHES) {
      num_switch_tests++;
      strcpy(switch_rec[num_switch_tests-1],sw);
    }
  }
  if(find_switch(sw)) { remove(argp); return TRUE; }
  return FALSE;
}
BOOL Args::find_switch(char *sw) {
  for(argp=1;argp<argc;argp++) {
    if(strcmp(argv[argp],sw)==0) { 
      last_switch = argv[argp];
      return TRUE;
    }
  }
  return FALSE;
}

void Args::goto_first() { argp=1; }

char* Args::pop_next() {
  if(argp>=argc) return NULL;
  char* save = argv[argp];
  remove(argp);
  return save;
}

char* Args::peek_next() {
  if(argp>=argc) return NULL;
  return argv[argp];
}

double Args::pop_number() {
  char* arg = pop_next();
  if(arg==NULL || !str_is_number(arg)) {
    if(arg && arg[0]=='0' && arg[1]=='x' && str_is_number(&arg[2]))
      { int hexnum; sscanf(arg,"%x",&hexnum); return hexnum; }
    uerror("Missing numerical parameter after '%s'", last_switch);
  }
  return atof(arg);
}

// if pos is present, value set to TRUE; if neg is present, value set to FALSE
// if neither, value is unchanged; if both, value is set to FALSE;
void Args::undefault(BOOL *target,char* pos,char* neg) {
  BOOL pp = extract_switch(pos), np = extract_switch(neg);
  if(pp) { *target = !np; } else if(np) { *target = FALSE; }
}


/*****************************************************************************
 *  STRING UTILITIES                                                         *
 *****************************************************************************/
// does the string contain a number after any initial whitespace?
BOOL str_is_number(char* str) {
  if(str==NULL) return FALSE;
  int i=0;
  while(isspace(str[i])) { i++; } // remove whitespace
  if(str[i]=='+' || str[i]=='-') i++; // initial plus
  BOOL decpt=FALSE, expt=FALSE;
  do {
    if(!isdigit(str[i])) { // digits OK
      if(str[i]=='.' && !decpt && !expt) decpt=TRUE; // optional decimal pt
      else if(str[i]=='e' || str[i]=='E' && !expt) expt=TRUE; // opt. exponent
      else return FALSE;
    }
    i++;
  } while(str[i] && !isspace(str[i]));
  return TRUE;
}

const char* bool2str(BOOL b) {
  static char truestr[]="TRUE", falsestr[]="FALSE";
  return b ? truestr : falsestr;
}
const char* flo2str(float num, int precision) {
  static char buffer[32]="", tmpl[10]="";
  sprintf(tmpl,"%s%d%s","%.",precision,"f");
  //printf("\nflo2str: %s\nFor precision: %d\n",tmpl,precision);
  sprintf(buffer,tmpl,num); return buffer;
}
const char* int2str(int num) {
  static char buffer[32]="";
  sprintf(buffer,"%d",num); return buffer;
}

/*****************************************************************************
 *  POPULATION CLASS                                                         *
 *****************************************************************************/
int Population::add(void* item) {
  if(removed.empty()) {
    if(top==capacity) resize_pop(capacity*2);
    store[top]=item; pop_size++; top++;
    return top-1;
  } else {
    int slot = removed.front(); removed.pop();
    if(store[slot]!=NULL) uerror("Population tried to over-write a node!");
    store[slot]=item; pop_size++;
    return slot;
  }
}

void* Population::remove(int i) {
  void* elt = store[i];
  if(elt) { store[i]=NULL; removed.push(i); pop_size--; }
  return elt;
}

void Population::destroy(int i) {
  void* elt = remove(i);
  if(elt) { free(elt); }
}

void Population::clear() {
  for(int i=0;i<top;i++) { store[i]=NULL; }
  while(!removed.empty()) removed.pop(); // clear the queue
  top=pop_size=0;
}

void* Population::get(int i) { return store[i]; }
void Population::init_pop(int cap) {
  store = (void**)calloc(cap,sizeof(void*));
  capacity = cap; top=0; pop_size=0;
}

void Population::resize_pop(int new_cap) {
  store = (void**)realloc(store,new_cap*sizeof(void*));
  if(store==NULL) uerror("Population fails to reallocate!");
  capacity=new_cap;
}
#ifdef __WIN32__
#include <windows.h>
double get_real_secs () {
  DWORD tv = timeGetTime();
  return (double)(tv / 1000.0);
}
#else
#include <sys/time.h>
double get_real_secs () {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)(t.tv_sec + t.tv_usec / 1000000.0);
}
#endif

// DLLUtil methods
void* DllUtils::dlopenext(const char *name, int flag)
{
  //const char **ext = dl_exts;
  void *hand = NULL;
  string prefix = "unkown_lib_prefix";
  string ext = "unknown_lib_extension";

#ifdef __CYGWIN__
    prefix = "";
    ext = ".dll";
  #else // assume other platforms to be Unix/Linux
    prefix = "lib";
    #ifdef __APPLE__
        ext = ".dylib" ;
    #else // assume all other platforms have .so extension
        ext = ".so";
    #endif
#endif

    stringstream ss;
    ss << prefix << string(name) << ext;
    string libFileName = ss.str();

    //cout << "Trying to load dll: " << libFileName << endl;
    hand = dlopen(libFileName.c_str(), flag);

  return hand;
}
