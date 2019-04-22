HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl\
					 -lglog
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = *.proto

vpath %.proto $(PROTOS_PATH)

all: backend clientcommandline servicelayertest keyvaluestoreinstance keyvaluestoretest servicelayer

backend: chirp.pb.o chirp.grpc.pb.o kvstore/backend.o kvstore/keyvaluestoreserver.cc kvstore/keyvaluestoreserver.h kvstore/test/keyvaluestoreinstance.o kvstore/test/keyvaluestoreinstance.h
	$(CXX) $^ $(LDFLAGS) -o $@ -lgtest

clientcommandline: chirp.pb.o chirp.grpc.pb.o commandline/clientcommandline.o commandline/clientfunctionalities.o commandline/clientfunctionalities.h
	$(CXX) $^ $(LDFLAGS) -o $@ -lgtest -lgflags

servicelayer: chirp.pb.o chirp.grpc.pb.o service/servicelayer.o service/servicelayerfunctionalities.o service/servicelayerfunctionalities.h service/clientforkeyvaluestore.o service/clientforkeyvaluestore.h
	$(CXX) $^ $(LDFLAGS) -o $@ -lgtest

keyvaluestoreinstance: kvstore/test/keyvaluestoreinstance.o kvstore/test/keyvaluestoreinstance.h
	g++ -std=c++11 -c -o kvstore/test/keyvaluestoreinstance.o kvstore/test/keyvaluestoreinstance.cc

keyvaluestoretest: kvstore/test/keyvaluestoretest.o kvstore/test/keyvaluestoreinstance.o kvstore/test/keyvaluestoreinstance.h
	$(CXX) $^ $(LDFLAGS) -o $@ -lgtest

servicelayertest: chirp.pb.o chirp.grpc.pb.o service/test/servicelayertest.o service/test/servicelayerinstance.o service/test/servicelayerinstance.h kvstore/test/keyvaluestoreinstance.o kvstore/test/keyvaluestoreinstance.h
	$(CXX) $^ $(LDFLAGS) -o $@ -lgtest

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` chirp.proto

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) $(PROTOS_PATH) --cpp_out=./

clean:
	rm -f *.o *.pb.cc *.pb.h backend clientcommandline servicelayer keyvaluestoreinstance keyvaluestoretest servicelayertest clientforkeyvaluestore
