OpenGl Renderer Plugin
-----

**Note**: This project is incredibly young, and isn't in any kind of production ready state. 

To clone:

    git clone --recursive https://github.com/icebreakersentertainment/opengl_renderer_plugin.git

Updating submodules:

    git submodule update --recursive --remote

Get/build prerequisites:

    cd ice_engine/ice_engine
    python setup.py

To build on Linux:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

To build on Windows:

    mkdir build
    cd build
    
    cmake -DCMAKE_BUILD_TYPE=Release ..
    msbuild /p:Configuration=Release opengl_renderer_plugin.sln
