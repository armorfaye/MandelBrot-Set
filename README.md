# MandelBrot-Set

This is a MandelBrot Set Fractal generator that can generate a png image or display it on a local host web server. 

Please download the following file:
emcompile.sh
index.html
mandelbrot.cpp
mandelbrot_web.cpp

The mandelbrot.cpp is the program used to generate the png and the second one uses emscripten to generate a web assembly for local host web server. The reason for using two separate C++ file is because javascript doesn't allow multithreading hence the first program employs multihreading to speed up the program.  



```

```
