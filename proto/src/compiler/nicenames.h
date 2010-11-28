#ifndef __NICENAMES__
#define __NICENAMES__

#include <string>

using namespace std;

class Nameable {
 private:
  string assigned_name;
 public:
  Nameable();
  string nicename();
};

#endif // __NICENAMES__
