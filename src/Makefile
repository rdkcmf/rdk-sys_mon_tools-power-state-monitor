##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
BASE_PATH=${BUILD_PATH}
CROSS_TOOLCHAIN=$(BASE_PATH)/workXG1/tools/stbgcc-4.5.4-2.8
CROSS_COMPILE=$(CROSS_TOOLCHAIN)/bin/mipsel-linux-uclibc
SDK_FSROOT=$(BASE_PATH)/sdk/fsroot/ramdisk

LDFLAGS+=$(GLIBS)
LDFLAGS+=$(DS_LIBS)
CXXFLAGS=-Wall ${ADDED_INCLUDE}

all : pwr-state-monitor

pwr-state-monitor:
	@echo "Building $@ ...."
	$(CC) $(CFLAGS) -o pwr-state-monitor powerStateMonitorMain.c $(LDFLAGS) $(DS_LIBS) $(GLIBS) -lIARMBus

clean:
	rm -rf bin *.o
install :
	mkdir bin
	mv pwr-state-monitor bin

