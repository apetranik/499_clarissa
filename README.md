# 499_clarissa

Cindy Clarissa
Clarissa@usc.edu
CSCI 499

**Forked by Aliya Petranik
petranik@usc.edu - added Stream(hashtag) functionality**

Compile with 'make'.
Run ./servicelayer
Run ./backend
Run ./clientcommandline

## 1. Vagrantfile (included in project)

    Download vagrant first:
    https://app.vagrantup.com/generic/boxes/ubuntu1804

## 2. Set-up steps

    1. Download vagrant.
    2. Replace vagrant file with my current vagrantfile
    3. Download vagrant: https://www.vagrantup.com/downloads.html
    	$ vagrant init project
    	a vagrantfile is placed, replace it with the one I have already provided.
    	$ vagrant up
    	$ vagrant reload
    	$ vagrant ssh
    4. Place 499_clarissa folder inside 'project'
    5. Install:
    	$ sudo apt install make
    	$ sudo apt install gcc
    	$ sudo apt install clang
    	$ sudo apt install g++
    	$ sudo apt install build-essential
    6. $ sudo apt-get install build-essential curl file git
    7. Install protobuf: https://github.com/grpc/grpc/blob/master/BUILDING.md
    	$ git clone https://github.com/protocolbuffers/protobuf.git
    	$ sudo apt-get install build-essential autoconf libtool pkg-config
    	$ sudo apt-get install libgflags-dev libgtest-dev
    	$ sudo apt-get update
    	$ sudo apt-get install clang libc++-dev
    		$ sudo apt-get -y install make cmake

    	$ sudo apt-get install pkg-config

    	$ git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc

    	Inside /vagrant/grpc:
    	$ git submodule update --init
    	$ make
    	$ sudo make install

    	then:
    	$ cd third_party/protobuf
    	$ make && sudo make install

    install gflags and gtest and glog
    	$ sudo apt-get -y install libgtest-dev
    	$ sudo apt-get -y install libgflags-dev
    	$ sudo apt-get -y install libgoogle-glog-dev
    	$ sudo apt-get install cmake
    	$ cd /usr/src/gtest
    	$ sudo cmake CMakeLists.txt
    	$ sudo make
    	$ sudo cp *.a /usr/lib

## 3. Complication instructions

    In 'project', type 'make' to compile.
    Then, ./backend for key value store
    ./servicelayer for the serveice layer
    ./clientcommandline for client commandline

## 4. Basic example usage

    Once ./backend and ./servicelayer is running, you can make commands:

    ./clientcommandline --register cindy
    	{Registered user: cindy}


    ./clientcommandline --user cindy --chirp "My first chirp."
    	{Succesfully chirped with id: <id>}


    ./clientcommandline --register damian
    	{Registered user: damian}

    ./clientcommandline --register cat --chirp "is it going to work?"
    	{Syntax Error. Please try again.}

    	./clientcommandline --user "aliya" --chirp "#hello!"

    	./clientcommandline --user "aliya" --stream "hello"
    	Note: Stream(hashtag) should NOT include the '#' character

    	Other notes about chirping w/ hashtags
    	  * whitespace is NOT necessary before hashtag if it appears at the start of a chirp (eg: "#hashtag hello!")
    	  * hashtag can terminate chirp WITHOUT whitespace (eg: "#hashtag")
    	  * hashtag MUST have whitespace char before and/or after it if characters precede or follow the hashtag (eg: "hello #hashtag helloagain")
    	  * Chirps can contain multiple hashtags as long as they are separated by a whitespace character
    	  * Super strings of hashtags do NOT work (eg: "#hashtag#hello") would be seen as hashtag: "hashtag#hello"
    	  * Stream(hashtag) supports 1 hashtag at a time per commandline instance (though you can open up multiple shells of course)
