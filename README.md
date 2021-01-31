# Simple-executable-profiler
Measures running time of a process with redirectable stdin, stout and stderr

## Building notice
Can be built with most compilers allowing C++ extensions. Built over C++17 with Boost

## Running
The program accepts several command line parameters
1. Relative path to exe  -- **non-optional, this parameter must be specified first**
2. Any combination of '--in', '--out', '--err' and '--count', followed by relative paths to files where stdin, stdout and stderr should be read from/redirected to, or a non-negative non-zero number to specify the amount of testing cycles for measurement. 
3. 'process', followed by any number of arguments to be passed to the executable -- **must be last. --in, --out and --err cannot be used after specific the process argument**
