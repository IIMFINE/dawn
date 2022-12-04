## Complie and install
You need
1. CMake version at least 3.10.x in linux
2. C++17 compiler
3. spdlog

There are two ways to build this project.

### On console directly 
For .e.g in unix os
```shell
$ mkdir build
$ cd build
$ cmake ..
$ make #The output executable files are in the fold named bin 
```

### In docker(recommend)
```shell
# First, pull a laster image. docker pull simplepan/dawn_dev
# Second, you have to attach to the container
$ cd {your directory stored repository}
$ mkdir build && cd build
$ cmake ..
$ make
```
We provide a `sudo` permission account `DAWN`. Password is `123456`.
