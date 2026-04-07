CXX      = g++
CXXFLAGS = -std=c++17 -Wall -pthread
TARGET   = worker

SRC = main.cpp task.cpp worker.cpp worker_pool.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
