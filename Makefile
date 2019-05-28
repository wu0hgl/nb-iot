CXX = g++
CXXFLAGS = -c --std=c++11 -Wall -Wno-unused-variable -Wno-unused-function -fPIC
LD	= g++
RM	= rm
LIBS = -larmadillo

5G_SIM_DEBUG = 5G-simulator
5G_SIM_RELEASE = 5G-simulator-release
FAST_FADING_LIBRARY = fast-fading.so

FAST_FADING_PATH = src/channel/propagation-model/FastFadingRealization

SRC_PROGRAM = $(shell find src/ -path $(FAST_FADING_PATH) -prune -o -name *.cpp -print)
SRC_FAST_FADING = $(shell find $(FAST_FADING_PATH) -name *.cpp)

OBJ_PROGRAM_DEBUG = $(SRC_PROGRAM:%.cpp=.obj/Debug/%.o)
OBJ_PROGRAM_RELEASE = $(SRC_PROGRAM:%.cpp=.obj/Release/%.o)
OBJ_FAST_FADING = $(SRC_FAST_FADING:%.cpp=.obj/Debug/%.o)

all: Debug

.obj/Debug/%.o: %.cpp
	@mkdir -p $(@D)
	@echo $<
	@$(CXX) -O0 -g3 -D_GLIBCXX_DEBUG -DDEBUG $(CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -MT"$(@:%.o=%.d)" $< -o $@

.obj/Release/%.o: %.cpp
	@mkdir -p $(@D)
	@echo $<
	@$(CXX) -O2     $(CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -MT"$(@:%.o=%.d)" $< -o $@

$(FAST_FADING_LIBRARY): $(OBJ_FAST_FADING)
	@$(LD) -shared -o $(FAST_FADING_LIBRARY) $(OBJ_FAST_FADING)

DEP = $(OBJ_PROGRAM_DEBUG:%.o=%.d)
-include $(DEP)

Debug: $(OBJ_PROGRAM_DEBUG) $(FAST_FADING_LIBRARY)
	@echo $(5G_SIM_DEBUG)
	@$(LD) -o $(5G_SIM_DEBUG) -Wl,-rpath=. $(OBJ_PROGRAM_DEBUG) $(LIBS) $(FAST_FADING_LIBRARY)

Release: $(OBJ_PROGRAM_RELEASE) $(FAST_FADING_LIBRARY)
	@echo $(5G_SIM_RELEASE)
	@$(LD) -o $(5G_SIM_RELEASE) -Wl,-rpath=. $(OBJ_PROGRAM_RELEASE) $(LIBS) $(FAST_FADING_LIBRARY)

clean:
	$(RM) -rf .obj/*

