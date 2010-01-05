/* a flex scanner for reading a C file containing one platform enum
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.
*/

%{
#define YY_BREAK { if(oecur->error) return 1; } break;
#define YY_DECL int zzlex( void )

#include <list>
#include <string>
#include "compiler-utils.h"
#include <stdio.h>

extern "C" int zzwrap() { return 1; }

int zzlex();

struct EnumLexer {
  bool ready_for_entry, error; int step; list<string> *p_ops;
  void read_error(string err) { *cperr << "Platform description file is not a parsable 'single enum' file: " << err << " .\n"; error=true; }

  string ibuf;
  EnumLexer(istream *in, ostream *out) {
    ready_for_entry=true; error=false; step=0; p_ops = new list<string>;
    // setup input stream; output is discarded
    yyout = tmpfile();
    ibuf = "";
    while(in->good()) ibuf+=in->get();
    ibuf[ibuf.size()-1]=0;
    yy_scan_string(ibuf.c_str());
  }
  virtual ~EnumLexer() {} // nothing to clean up: p_ops is somebody else's prob
  
  // returns NULL on error
  list<string>* tokenize() {
    yylex();
    if(!error && step<9) { read_error("file not closed by ';'"); }
    if(error) { delete p_ops; return NULL; 
    } else { return p_ops; }
  }
};

EnumLexer* oecur;

%}


%%

[[:space:]]+	/* consume whitespace */
"//".*$		/* consume C++ comments */
"/*"	        { register int c; /* consume C comments */
                  for ( ; ; ) {
		    while ( (c = yyinput()) != '*' && c != EOF )
		      ;	   /* eat up text of comment */
		    if ( c == '*' ) {
		      while ( (c = yyinput()) == '*' ) ;
		      if ( c == '/' )
			break;    /* found the end */
		    }
		    if ( c == EOF ) {
		      oecur->read_error("file ended w. unclosed C comment");
		      break;
		    }
  		  }
		}

"typedef"	    { if(oecur->step==0) oecur->step++; else oecur->read_error("wrong location of 'typedef'"); }
"enum"		    { if(oecur->step==1) oecur->step++; else oecur->read_error("wrong location of 'enum'"); }
"{"		    { if(oecur->step==2) oecur->step++; else oecur->read_error("wrong location of '{'"); }
"="		    { if(oecur->step==3) oecur->step++; else oecur->read_error("wrong location of '='"); }
"CORE_CMD_OPS"      { if(oecur->step==4) oecur->step++; else oecur->read_error("wrong location of 'CORE_CMD_OPS'"); }
"MAX_CMD_OPS"       { if(oecur->step==5) oecur->step++; else oecur->read_error("wrong location of 'MAX_CMD_OPS'"); }
"}"		    { if(oecur->step==6) oecur->step++; else oecur->read_error("wrong location of '}'"); }
"PLATFORM_OPCODES"  { if(oecur->step==7) oecur->step++; else oecur->read_error("wrong location of 'PLATFORM_OPCODES'"); }
";"		    { if(oecur->step==8) oecur->step++; else oecur->read_error("wrong location of ';'"); }

,		{ if(oecur->ready_for_entry) oecur->read_error("bad comma location"); else oecur->ready_for_entry=true; }

[[:alnum:]\_]+  { if(!oecur->ready_for_entry || 
			        (oecur->step==2 && !oecur->p_ops->empty()) ||
                                (oecur->step==4 && oecur->p_ops->empty())) oecur->read_error("Bad constituent");
			     else { oecur->p_ops->push_back(string(yytext)); 
			       oecur->ready_for_entry=false;
			     } }


%%

list<string>* read_enum(istream* in, ostream* out=NULL) { 
  string dump;
  if(out==NULL) out=new ostringstream();
  EnumLexer lex(in,out); oecur=&lex;
  return lex.tokenize();
}
list<string>* read_enum(string in) { 
  return read_enum(new istringstream(in));
}
