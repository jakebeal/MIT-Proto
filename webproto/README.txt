To Install on a Server:

1) svn co <PROTO/trunk>
2) compile proto (via instructions in SVN repo)
3) copy webproto/webcompiler into CGI directory (e.g., /usr/lib/cgi-bin) and edit as necessary



This is a highly experiment scratch project, and should be treated with extreme
dubiousness.

-JSB, 6/21/13


README
Kyle Usbeck <kusbeck@bbn.com>
6/26/2013

webproto.js is an emscripten-compiled version of the Proto VM,
specifically DelftProtoVM.  Acquire the modified version of
DelftProtoVM (where certain components are made available to
JavaScript via emscripten bind (i.e., embind), and compile using:

> emmake make webproto.js

p2b is a python script that is meant to handle calls via CGI.
put it in your system's CGI folder (e.g., /usr/lib/cgi-bin),
make sure to enable CGI, and point it to your JSON-enabled
version of the proto p2b

ace is the library that we're using to display/edit code.
We added a syntax highlighting module, ace/mode-proto.js
