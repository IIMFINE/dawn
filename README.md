# Dawn

## Compile and Install

You need

1. CMake version at least 3.10.x in linux
2. C++17 compiler
3. spdlog
4. boost

There are two ways to build this project.

### On Console Directly

For .e.g in unix os

```shell
mkdir build
cd build
cmake ..
make #The output executable files are in the fold named bin
make install
```

### In Docker(recommend)

```shell
# First, pull a laster image. docker pull simplepan/dawn_dev
# Second, you have to attach to the container
cd {your directory stored repository}
mkdir build && cd build
cmake ..
make
make install
```

We provide a `sudo` permission account `DAWN`. Password is `123456`.

### Build Docker Image

```shell
cd Docker
docker build --network=host --no-cache --target build_base -t dawn_base:latest
docker build --network=host --no-cache --target build_dependence -t image_name
```

## Usage

1. In your CMakeLists.txt, find this package and depend on it.

    ```cmake
    find_package(dawn REQUIRED)
    target_link_libraries(your_target dawn::dawn)
    ```

2. In your C++ source file, include the header file and use API.

    ```cpp
    //Process 1
    #include <dawn/transport/shmTransport.h>
    int main()
    {
      dawn::shmTransport tp("dawn");
      tp.write(data, data_len);
    }

    //Process 2
    #include <dawn/transport/shmTransport.h>
    int main()
    {
      dawn::shmTransport tp("dawn");
      int data_len;
      //Variable data_len will be passed as a reference.
      tp.read(data, data_len, dawn::BLOCK_TYPE::BLOCK);
      //Receive data from process 1.
    }
    ```

### Note

1. Dawn support multiple publishers and multiple subscribers.
2. Dawn shmTransport bases on share memory.
