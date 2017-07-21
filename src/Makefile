CURRENT_DIR:=$(shell pwd)

LD_LIB= $(CURRENT_DIR)/../3rd/install/lib/libavformat.a $(CURRENT_DIR)/../3rd/install/lib/libavcodec.a $(CURRENT_DIR)/../3rd/install/lib/libavutil.a $(CURRENT_DIR)/../3rd/install/lib/libswresample.a $(CURRENT_DIR)/../3rd/install/lib/libx264.a $(CURRENT_DIR)/../3rd/install/lib/libgflags.a 
LD_INCLUDE=$(CURRENT_DIR)/../3rd/install/include
FILE_OUT=transtools
#LIB_OUT=libdetect_md.so

CC=g++ -g
CC_SHARE_OUT=gcc -fPIC -shared -o 
CC_OUT=g++ -g -o
CC_FLAGS= -fPIC -I$(LD_INCLUDE) 
LD_FLAGS= $(LD_LIB) -lz -lm -lpthread 
LD_FLAGS1=-Wl,-Bsymbolic
CC_DEPFLAGS=-MMD -MF $(@:.o=.d) -MT $@
all:$(FILE_OUT)
#all:$(LIB_OUT)
FILE_C=$(wildcard *.c)
FILE_CPP=$(wildcard *.cpp)

FILE_O=$(patsubst %.c, %.o, $(FILE_C))
FILE_O+=$(patsubst %.cpp, %.o, $(FILE_CPP))
FILE_D=$(patsubst %.c, %.d, $(FILE_C))
FILE_D+=$(patsubst %.cpp, %.d, $(FILE_CPP))

%.o:%.c %.cpp
	$(CC) $(CC_FLAGS) $(CC_DEPFLAGS) -c  $< -o $@

$(FILE_OUT):$(FILE_O)
	$(CC_OUT) $(FILE_OUT) $(FILE_O) $(LD_FLAGS)
#$(LIB_OUT):$(FILE_O)
#	$(CC_SHARE_OUT) $(LIB_OUT) $(FILE_O) $(LD_FLAGS) $(LD_FLAGS1)

clean:
	rm -f $(FILE_O) $(FILE_D) $(FILE_OUT)
