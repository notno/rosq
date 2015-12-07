#include <stdio.h>

void hello (int n) {
    while (n > 0) {
        puts("%i It's a Hell World", n);
        n--;
    }
    
}

int main(int argc, char** argv) {
    hello(123);
    return 0;
}