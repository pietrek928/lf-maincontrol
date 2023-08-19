// g++ test_binary_logging.cc -o test_binary_logging.e -O2 -s && ./test_binary_logging.e
#include <cstdio>
#include <cstdint>

#define BINARY_LOG_LOCATION "."
#define likely(v) v

#include "binary_logging.h"

using namespace std;

int main() {
    for (int i=0; i<100; i++) {
        int b = i + 2;
        LOG_VALUES(i, b);
    }
    return 0;
}
