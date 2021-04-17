CXX=g++-10
BUILD_DIR=build
CPPFLAGS=-std=c++20

ifdef DEBUG
	CPPFLAGS=-g
endif

default: test

test:
	mkdir -p $(BUILD_DIR)
	$(CXX) -Wall $(CPPFLAGS) -o $(BUILD_DIR)/ThreadPoolTest ThreadPoolTest.cpp -lgtest_main -lgtest -lpthread -pthread
	$(BUILD_DIR)/ThreadPoolTest

install:
	install ThreadPool.h /usr/local/include/

doc:
	doxygen

test-valgrind:
	$(CXX) -Wall $(CPPFLAGS) -g -o $(BUILD_DIR)/ThreadPoolTest ThreadPoolTest.cpp -lgtest_main -lgtest -lpthread -pthread
	valgrind --leak-check=full $(BUILD_DIR)/ThreadPoolTest

clean:
	rm -rf $(BUILD_DIR)
	rm -rf html
	rm -rf latex