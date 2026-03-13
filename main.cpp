#include <iostream>
#include <string>
#include "core.h"

int main() {
    try {
        const std::string msg = "hello world";

        auto keys = KeyGenFlat();
        auto p = keys.find('|');
        std::string pk = keys.substr(0, p);
        std::string sk = keys.substr(p + 1);

        auto b = Blind(pk, msg);
        auto s = b.find(':');
        std::string blinded = b.substr(0, s);
        std::string r_inv = b.substr(s + 1);

        std::string sig = Finalize(pk, msg, BlindSign(sk, blinded), r_inv);

        std::cout << (Verify(pk, msg, sig) ? "OK\n" : "FAIL\n");

    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}