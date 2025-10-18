#include <iostream>
#include <libpd/PdBase.hpp>

int main() {
    try {
        pd::PdBase a;
        pd::PdBase b;
        std::cout << "PdBase::numInstances() = " << pd::PdBase::numInstances() << std::endl;
        // Try a minimal init path if available; guard in case PdBase::init requires libpd built in a certain way
        bool ai = a.init(0, 2, 44100);
        bool bi = b.init(0, 2, 44100);
        std::cout << "a.init()=" << ai << " b.init()=" << bi << std::endl;
        a.clear();
        b.clear();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 2;
    }
}
