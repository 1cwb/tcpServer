# for sub directory
export SRC_AMSFILES :=
export SRC_C_FILES :=
export SRC_INCFILES :=
export SRC_CXX_FILES :=
export SRC_HPP_FILES :=
export SRC_INCDIR :=
export LINK_FILES :=
export BOARD_TYPE :=

# 要编译的文件夹，用空格分隔
ifdef SERVER
BUILD_DIR := $(CURDIR)/server
else
BUILD_DIR := $(CURDIR)/client
endif

#$(info "$(BUILD_DIR)")
SUBDIRS := $(shell find $(BUILD_DIR) -maxdepth 4 -type d)
SUBMK += $(foreach dir, $(SUBDIRS), $(wildcard $(dir)/*.mk))
include $(SUBMK)
#$(shell sleep 5)
################################################################################################
OUTPUT := $(CURDIR)/output
ifdef SERVER
TARGET           	?= server
else
TARGET           	?= client
endif
OUTPUTDIR += $(OUTPUT)

define ensure_dir
    @if [ ! -d "$(1)" ]; then \
        echo "创建目录: $(1)"; \
        mkdir -p $(1); \
    else \
        echo "目录已存在: $(1)"; \
    fi
endef

###################################################COMPILE######################################
CROSS_COMPILE    	?=

CC               	:= $(CROSS_COMPILE)gcc
CXX              	:= $(CROSS_COMPILE)g++
LD               	:= $(CROSS_COMPILE)ld
OBJCOPY          	:= $(CROSS_COMPILE)objcopy
OBJDUMP          	:= $(CROSS_COMPILE)objdump
SIZEINFO            := $(CROSS_COMPILE)size

#############################################################ARM################################


##################################################COMPILE_FLAGS#################################
C_COMPILE_FLAGS 	:= -lc -lm -lnosys -std=c11 -Wall -fdata-sections -ffunction-sections -g0 -gdwarf-2 -Os
CXX_COMPILE_FLAGS 	:= -std=c++17
#################################################################################################
EXTRA_LINK_FLAGS	:= 
#################################################################################################
DEFINE :=
###############################################################
CFLAGS 				+= $(C_COMPILE_FLAGS) $(DEFINE)
CXXFLAGS 			+= $(CXX_COMPILE_FLAGS) $(DEFINE)

LFLAGS +=  $(EXTRA_LINK_FLAGS) -lstdc++ -lpthread

INCLUDE 		 := $(patsubst %, -I %, $(SRC_INCDIR))
SFILES			 := $(SRC_AMSFILES)
CFILES			 := $(SRC_C_FILES)
CXXFILES         := $(SRC_CXX_FILES)
HPPFILES         := $(SRC_HPP_FILES)

SFILENAME  		 := $(notdir $(SFILES))
CFILENAME 		 := $(notdir $(CFILES))
CXXFILENAME		 := $(notdir $(CXXFILES))
HPPFILENAME      := $(notdir $(HPPFILES))

SOBJS		 	 := $(patsubst %, $(OUTPUTDIR)/%, $(SFILENAME:.s=.o))
COBJS		 	 := $(patsubst %, $(OUTPUTDIR)/%, $(CFILENAME:.c=.o))
CXXOBJS          := $(patsubst %, $(OUTPUTDIR)/%, $(CXXFILENAME:.cpp=.o))
HPPOBJS          := $(patsubst %, $(OUTPUTDIR)/%, $(HPPFILENAME:.hpp=.o))
OBJS			 := $(SOBJS) $(COBJS) $(CXXOBJS)

SRCDIRS          := $(dir $(SFILES)) $(dir $(CFILES)) $(dir $(CXXFILES))
VPATH			 := $(SRCDIRS)

.PHONY: clean


#$(info "SFILES = $(SFILES) ")
#$(info "CFILES = $(CFILES) ")

$(OUTPUTDIR)/$(TARGET).elf:$(OBJS)
	$(CC) -o $(OUTPUTDIR)/$(TARGET).elf $^ $(LFLAGS)
	$(OBJCOPY) -O binary -S $(OUTPUTDIR)/$(TARGET).elf $(OUTPUTDIR)/$(TARGET).bin
	$(OBJDUMP) -D $(OUTPUTDIR)/$(TARGET).elf > $(OUTPUTDIR)/$(TARGET).dis
	$(SIZEINFO) $@

$(COBJS) : $(OUTPUTDIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

$(CXXOBJS) : $(OUTPUTDIR)/%.o : %.cpp
	$(call ensure_dir,$(OUTPUTDIR))
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o $@ $<

$(HPPOBJS) : $(OUTPUTDIR)/%.o : %.hpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o $@ $<

clean:
	rm -rf $(OUTPUT)/*