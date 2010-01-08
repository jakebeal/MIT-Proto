#! /usr/local/bin/python
''' prototest regression tester for Proto
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.
'''

'''

Prototest helps analyze dump files to perform regression testing on
the proto language core. It reads config file(s) containing "tests"
each with one or more "assertions", and verifies the assertions. The
results are stored in a log file.  Look at proto_readme for a long
help including tutorial on how to create config files, or run
prototest with -h parameter to see how to use prototest.

USAGE:
python prototest.py <TestName> [one or more switches]
'''

import linecache, optparse, os, random, subprocess, sys
prototest_version = "Prototest 0.75"
#Global Variables
verbosity = 1
dump_dir = ""
recursive_dir_scan = False

#Exceptions
class MissingDumpFileError(Exception):
    '''Thrown when a test cannot find its dumpfile'''
    pass

class InvalidConfigPathError(Exception):
    '''
    Thrown when user passes a file that is not a testfile
    or if the directory doesn't contain any other test files
    '''
    pass

#The Hierarchy is:
#Test Suite -> Test File -> Test -> Assertion (-> means one or more)

class Assertion():
    '''
    Assertions compare expected value to actual values in the dump file.
    They contain the comparison function, along with the line and col number
    to read the actual value.
    '''

    def __init__(self, line, column, expected_val, comparison_fn, desc, is_numeric = True):
        '''
        Initialized using line & column number to find the actual value, the
        expected value, the function to compare the two values, and a string
        describing the type of assertion.
        '''
        #Cast to ints & floats because parser passes strings
        self.line = int(line)
        self.col = int(column) if column.isdigit() else "_"
        self.is_numeric  = is_numeric
        self.expected_val = float(expected_val) if is_numeric else expected_val
        self.comparison_fn = comparison_fn
        self.desc = str(desc)
        self.dumpfile = "" #is known only after the tests are run

        #Failed means value didn't match expected.
        #Crashed means couldn't read value (etc.)
        self.failed = False #Default starting value
        self.actual_val = 0 if is_numeric else expected_val
        self.crash = False
        self.crash_reason = ""

    def run(self, dumpfile):
        '''
        Reads the actual value from the given dumpfile, and compares it to the
        expected value.
        '''
        self.dumpfile = dumpfile
        #Read value
        try:
            #Use linecache to provide random-access to lines within the file
            #Add 1 to line num because linecache numbers from 1...
            lineval = linecache.getline(dumpfile, self.line + 1)

            if self.col == "_": #Test whole lines
                self.actual_val = lineval.strip()
            else:
                actual_val = lineval.split()[self.col]
                #Since linecache silently returns "" instead of raising exceptions:
                if actual_val == '' or actual_val == '\n':
                    raise IndexError
                self.actual_val = float(actual_val) if self.is_numeric else actual_val
        except IndexError:
            self.set_exception("\nIndex out of bounds (NOTE: Line/Column Numbering is done from 0) ")
        except ValueError:
            self.set_exception("Non-numeric value %s found (this assertion is numeric)" % actual_val)
        else:
            self.failed = not self.comparison_fn(self.actual_val, self.expected_val)

        return self.get_result()

    def set_exception(self, error_desc):
        ''' Called to set exception when trying to run assertion function '''
        self.crash = True
        self.crash_reason = "FAIL\t" + error_desc + \
                            "Trying to access line %d, column %s in %s" % (self.line, self.col, self.dumpfile)
        self.failed = True

    def get_result(self):
        ''' Returns a pretty-output version of the result. '''

        if self.crash:
            return self.crash_reason
        elif self.failed:
            out = "FAIL\t"
        else:
            out = "pass\t"
        out += "Value at Line %s Column %s (%s) %s %s " % \
               (self.line, self.col, self.actual_val, self.desc, self.expected_val)
        return out


class Test():
    '''
    A test is a collection of assertions made on a dump file generated by
    proto.
    '''

    def __init__(self, protoarg, dumpfile_prefix):
        '''
        A test is initialized by providing the arguments to run proto with,
        a prefix for the dump file (randomly generated by parser), and if set by
        user, a dump directory to find the dump file. The other attributes are
        filled in dynamically.
        '''
        self.protoarg = protoarg
        self.dumpfile_prefix = dumpfile_prefix
        #Assertions are added dynamically instead of at instantiation
        self.asserts = []
        self.fail_asserts = []
        self.reset()
        self.failed = False #Default Starting Value
        self.crash = False #Crashed = proto crashed / non-zero return code
        self.crash_reason = "" #String to describe an exception.
        self.proto_output = ("","") #stdout,stderr

    def add(self, assertion):
        '''Adds assertion to the test.'''
        self.asserts.append(assertion)

    def reset(self):
        '''Reset to before test situations'''
        self.fail_asserts = []
        self.failed = False
        self.crash = False
        self.crash_reason = ""
        self.proto_output = ("","")


    def find_dump(self):
        '''Returns path to the actual dump file with the specified prefix'''
        #Set dump directory (if user hasn't set one already)
        global dump_dir

        if not dump_dir:
            current_dir = os.path.abspath(sys.path[0])
            dump_dir = os.path.join(current_dir,'dumps')
        #Filter to find candidates for the dump file
        matches = [x for x in os.listdir(dump_dir) \
                   if x.startswith(self.dumpfile_prefix) and x.endswith('.log')]
        if len(matches) < 1:
            raise MissingDumpFileError("Searched for %s%s" % (self.dumpfile_prefix, ".log"))

        logname = matches[0] #Match is first item in the list
        #Prefix the dump directory, and return
        return os.path.join(dump_dir, logname)

    def test_crashed(self, crash_reason):
        '''
        Convenience method to be called when a test crashes.
        Marks all assertions as failed, and sets the crash reason.
        '''
        self.crash = True
        self.failed = True
        self.crash_reason = crash_reason
        #Mark all Assertions as failed
        for a in self.asserts:
            a.set_exception(crash_reason)
        self.fail_asserts.extend(self.asserts)

    def run(self):
        '''Executes proto, runs all assertions on the resulting dump file.'''
        self.reset()

        # substitute paths to the programs correctly
        protoarg = self.protoarg
        protoarg = protoarg.replace("$(PROTO)", proto_path)
        protoarg = protoarg.replace("$(P2B)", p2b_path)
        protoarg = protoarg.replace("$(DEMOS)", demos_path)

        try:
            #Create process, run & get return code
            p = subprocess.Popen(protoarg, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.proto_output = p.communicate()
            if p.returncode != 0:
                raise subprocess.CalledProcessError(p.returncode, self.protoarg)
            #Find dump file, run assertions
            dumpfile = self.find_dump()
            for a in self.asserts:
                a.run(dumpfile)
                if a.failed:
                    self.fail_asserts.append(a)
                    self.failed = True
        except subprocess.CalledProcessError, exc:
            error_str = "Proto terminated with a non-zero return code: " + str(exc)
            self.test_crashed(error_str)
        except MissingDumpFileError, exc:
            error_str = "Could not find the dumpfile. " + str(exc)
            print error_str
            self.test_crashed(error_str)
        except:
            #Just in case there is some other exception (we don't want the entire program to crash)
            self.test_crashed("Unhandled Exception: %s" % (sys.exc_info()[1])) #exec_info returns: (type, value, traceback)

    def get_result(self):
        ''' Returns a pretty-output version of the result. '''
        if self.crash:
            return self.crash_reason
        elif self.failed:
            out = ["FAIL: Failed %d out of %d assertions." \
                   % (len(self.fail_asserts), len(self.asserts))]
            for f in self.fail_asserts:
                out.append(f.get_result())
            return '\n'.join(out)
        else:
            return "All assertions passed."

class TestFile():
    ''' Represents all the tests contained in a single file.'''
    def __init__(self, filename):
        ''' initialized by giving it the name of the test file.'''
        self.filename = filename
        self.tests = []
        self.failed = False
        #To Write Dump Files:
        self.randomseed = random.randint(10000, 99999)

    def parse_file(self):
        ''' Builds and runs config parser on the test config file.'''
        self.config_parser = ConfigParser(self.filename)
        self.tests = self.config_parser.run()

    def run(self):
        '''Runs all tests in the test file.'''
        self.failed = False #reset
        print "Running %s  " % self.filename,
        tests_completed = 0
        for test in self.tests:
            test.run()
            if test.failed:
                self.failed = True
            #Print progress bar; one "." for every 5 tests completed.
            sys.stdout.flush()
            tests_completed += 1
            if tests_completed % 5 == 0 or tests_completed == 1:
                print "\b.",

        #Write Logs
        if self.failed:
            print " FAILED %d out of %d tests" % (len([t for t in self.tests if t.failed]), len(self.tests))
        else:
            print " passed all %d tests" % len(self.tests)

        self.write_log()
        return self.get_result()

    def get_result(self):
        '''Returns a pretty output'''
        tests_passed = len([t for t in self.tests if not t.failed])
        if tests_passed == len(self.tests):
            return "All tests Passed"
        else:
            return "Passed %d out of %d tests." % (tests_passed, len(self.tests))

    def write_log(self):
        ''' Generates a test log for the file.'''
        log = [self.get_result()]

        for i in range(len(self.tests)):
            test = self.tests[i]
            if verbosity >= 1 or test.failed:
                log.extend(["Test #%d" % i,
                            "Running with arguments: %s" % test.protoarg,
                            "Dump file path: %s" % test.asserts[0].dumpfile,
                            "\n***PROTO OUTPUT***",
                            ''.join(test.proto_output),
                            "***END PROTO OUTPUT***\n",
                            test.get_result()
                            ])
                log.append("-" * 80)

        out_file = os.path.basename(self.filename) + ".RESULTS"
        out = open(out_file,"w")
        out.write('\n'.join(log))
        out.close()


class TestSuite():
    '''
    A test suite is a collection of test files. It is provided to allow
    users to run a comprehensive test suite without having to create a
    monolithic test file.
    '''
    def __init__(self, tests):
        self.tests = tests
        self.test_files = {} #Store test_name: Test
        self.failed = False #Default Starting Value

    def add(self, test_filename, test):
        ''' Add a test file to the test suite. '''
        self.test_files[test_filename] = test

    def gen_tests(self):
        '''
        Populates test suite by adding all files with a ".test" extension
        to the test file.
        '''

        assert len(self.tests) != 0 # already checked elsewhere
        print "Found %d test files." % len(self.tests)

        for test_name in self.tests:
            t = TestFile(test_name)
            t.parse_file()
            if len(t.tests) == 0:
                print "Warning: No tests found in %s " % test_name
            else:
                self.add(test_name, t)

    def filter_tests(self, filelist, test_existence = False):
        '''
        Removes all files that don't end with '.test' extension. Also performs some
        cleanup: removes leading and trailing whitespace, removes duplicates, removes non-existant files.

        Intended to be used in directory mode or test suite mode
        '''
        cleaned = set([x.strip() for x in filelist \
                         if x.strip().lower().endswith('.test')])

        if test_existence:
            for f in cleaned:
                try:
                    z = open(f,"r")
                except IOERROR:
                    cleaned.remove(f)

        return cleaned

    def run(self):
        '''Runs all Test files in test suite.'''
        for test_file_name, test_file in self.test_files.items():
            test_file.run()
            if test_file.failed:
                self.failed = True
        return not self.failed

class ConfigParser():
    '''
    This class is responsible for parsing a "config file" which is a list of
    tests and assertions. Supported comparison operators are defined in the
    dictionaries below with a
        symbol: (function, description)
    format.

    NOTE ON DEFINING FNS WHICH REQUIRE EXTRA ARGUMENTS:
    Use currying to convert a function of n-arguments into a higher order
    function that accepts a list of all arguments other than "expected value"
    and "actual value". (See the definition of ~= for an example.)
    '''

    numeric_fns = {"=" : ((lambda act, exp: act == exp), " equal to "),
           ">"  : ((lambda act, exp: act > exp), " greater than "),
           "<"  : ((lambda act, exp: act < exp), " less than "),
           ">=" : ((lambda act, exp: act >= exp), "greater than / equal to "),
           "<=" : ((lambda act, exp: act <= exp), "less than / equal to "),
           "!=" : ((lambda act, exp: act != exp), " not equal to "),
           "~=" : ((lambda arg_list: (lambda act, exp: abs(act - exp) <= arg_list[0])),
                   " nearly equal to ")
           }

    string_fns = {"is_nan" : ((lambda act, exp: act.lower() == "nan"), 1, " is "),
                  "is" : ((lambda act, exp: act.lower() == exp.lower()), 0, " is "),
                  "has": ((lambda act, exp: act.lower().find(exp.lower()) != -1), 0, " has ")}

    def __init__(self, test_file):
        '''
        Initialized by giving it an instance of TestFile
        '''
        self.test_file = test_file
        self.tests = []
        self.randomseed = random.randint(10000, 99999)

    def reset(self):
        ''' Removes all tests parsed so far. '''
        self.tests = []

    def gen_dump_name(self):
        '''Attaches a prefix to the proto dump file.'''
        return "prototest" + str(self.randomseed + len(self.tests))

    def run(self):
        '''Scans the config file and populates test cases and assertions.'''
        self.reset()


        #Open config file
        try:
            config_file = open(self.test_file, "r")
        except UnboundLocalError:
            #This is the only way to know if the file couldn't be opened.
            print "Could not open the config file (IO Exception)"

        if verbosity > 0:
            print "Parsing %s" % self.test_file,
        #Parse line by line
        for line in config_file:
            line = line.rstrip("\n")
            #Split by whitespace, ignore empty lines
            split = line.split()
            if not split:
                continue

            cmd = split[0] #first token is the "command"

            if cmd == "test:":
                if verbosity >= 3:
                    print "PARSER: encountered a test: ", line
                #Generate a dump file name for this test
                dump_file_name = self.gen_dump_name()

                #Have proto use this dump file name:
                protoargs = split[1:]
                try:
                    protoargs.insert(protoargs.index("-D") + 1,
                                     r"-dump-stem " + dump_file_name)
                except ValueError:
                    #User hasn't provided a dump file related switch.
                    protoargs.append(r"-D -dump-stem " + dump_file_name)
                #If using -v0, automatically make all tests headless [exclude p2b]
                if not "-headless" in protoargs and not "--test-compiler" in protoargs and verbosity == 0:
                    protoargs.append("-headless")
                #Finally, create a test
                protoargs = ' '.join(protoargs)
                self.tests.append(Test(protoargs, dump_file_name))
            elif cmd.startswith("//"):
                if verbosity >= 3:
                    print "PARSER: encountered a comment: ", line
            elif cmd in self.numeric_fns:
                if verbosity >= 3:
                    print "PARSER: encountered a numeric assertion: ", line
                #<cmd> <line #> <column #> <expected val> <other args>
                if len(split[4:]) == 0:
                    #Binary function
                    fn = self.numeric_fns[cmd][0]
                else:
                    #Curried Function
                    fn = self.numeric_fns[cmd][0](split[4:])
                a = Assertion(split[1], split[2], split[3], fn,\
                              self.numeric_fns[cmd][1], is_numeric = True)
                self.tests[-1].add(a)
            elif cmd in self.string_fns:
                if verbosity >= 3:
                    print "PARSER: encountered a string assertion: ", line
                #If fn works on entire line, split properly & strip whitespace.
                if split[2] == "_":
                    expected = line.split(None,3)[3].strip()
                else:
                    expected = split[3]
                fn = self.string_fns[cmd][0]
                a = Assertion(split[1], split[2], expected, fn, self.string_fns[cmd][2], is_numeric = False)
                self.tests[-1].add(a)
            else:
                print "Parser: encountered unrecognized command: ", line

        self.tests = [t for t in self.tests if len(t.asserts) != 0]
        if verbosity > 0:
            print "... %d tests found" % len(self.tests)
        return self.tests



def main():
    '''Builds and runs tests. (Main Entry Point of program)'''
    print prototest_version

    #Build Command Line Parser
    parser = optparse.OptionParser(prog="prototest", version=prototest_version)
    parser.add_option("-v", "--verbose", type="int", action="store",
                      dest="verbosity", help="Prints diagnostic messages, more when used multiple times")
    parser.add_option("-r", "--recursive", action="store_true", dest="recursive", help="Looks for test files recursively in subdirectories.")
    parser.add_option("-d", "--dumpdir", dest="dump_dir",
                      help="Set the path to dump files. (use only if prototest is not in the same dir. as proto)")
    parser.add_option("--proto", dest="proto",
                      help="Path to the proto executable")
    parser.add_option("--p2b", dest="p2b",
                      help="Path to the p2b executable")
    parser.add_option("--demos", dest="demos",
                      help="Path to the demos")
    parser.set_defaults(verbosity=1, dumpdir="", proto="../proto", p2b="../p2b", demos="../../demos")

    #Parse Command Line Arguments
    (option, args) = parser.parse_args()
    if len(args) < 1:
        sys.exit("ERROR: You must provide at least one test file")
    else:
        global verbosity; global dump_dir; global recursive_dir_scan;
        global proto_path
        global p2b_path
        global demos_path
        (verbosity, dump_dir, recursive_dir_scan) = \
            (option.verbosity, option.dump_dir, option.recursive)
        proto_path = option.proto
        p2b_path = option.p2b
        demos_path = option.demos

    #Build and Run Tests
    test_suite = TestSuite(args)
    print "PARSING TEST FILE(s)"
    test_suite.gen_tests()
    print "RUNNING TEST(s)"
    if test_suite.run():
        sys.exit(0)
    sys.exit(1)

if __name__ == "__main__":
    main()
