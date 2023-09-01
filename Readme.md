The purpose of this small C++ benchmark is to measure the latency of cache synchronization. The main idea is to pin two different threads on specific cores and measure the time required for load/store atomic operations, including cache synchronization between these cores.

To run this benchmark you can follow the instruction:
cmake -B build
cmake --build build
./build/bench
