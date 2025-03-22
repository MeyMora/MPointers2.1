#include <iostream>
#include "MPointer.h.cpp"

int main() {
    MPointer<int>::Init();
    MPointer<int> ptr = MPointer<int>::New();

    ptr.setValue(42);
    std::cout << "Valor en memoria: " << ptr.getValue() << std::endl;

    return 0;
}



