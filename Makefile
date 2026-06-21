CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -g
LDFLAGS  =

TARGET   = test_avl
SRCS     = avl_tree.cpp test_avl.cpp
OBJS     = $(SRCS:.cpp=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp avl_tree.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild if any header changes
avl_tree.o: avl_tree.h
test_avl.o: avl_tree.h
