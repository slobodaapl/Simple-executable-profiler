# Simple-executable-profiler
Measures running time of a process with redirectable stdin, stout and stderr

## Building notice
Can be built with most compilers allowing C++ extensions. Built over C++17 with Boost

## Running
The program accepts several command line parameters
1. Relative path to exe **non-optional**
2. Any combination of '--in', '--out', '--err', followed by relative paths to files where stdin, stdout and stderr should be read from/redirected to
3. 'process', followed by any number of arguments to be passed to the executable **must be last**
