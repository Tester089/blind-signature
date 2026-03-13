#define BLIND_SIG_IMPLEMENTATION
#include "core.h"

#include <iostream>
#include <string>

void client_example() {
    std::string pk;
    std::cin >> pk;

    std::string msg = "Candidate_A";

    std::string blind_result = Blind(pk, msg);
    const std::size_t sep = blind_result.find(':');
    if (sep == std::string::npos) {
        std::cerr << "Bad blind result format\n";
        return;
    }

    std::string blinded_msg = blind_result.substr(0, sep);
    std::string inv = blind_result.substr(sep + 1);

    std::cout << "Blinded: " << blinded_msg << "\n";
    std::cout << "Inv: " << inv << "\n";

    std::cout << "Sending blinded_msg to server...\n";
    std::cout << "Enter blind_sig: ";

    std::string blind_sig;
    std::cin >> blind_sig;

    std::string sig = Finalize(pk, msg, blind_sig, inv);

    bool ok = Verify(pk, msg, sig);
    if (ok) {
        std::cout << "Nice!\n";
    } else {
        std::cout << "We have some problems...\n";
    }
}

void server_example() {
    KeyPair keys = KeyGen();

    std::string pk = keys.pk->toString();
    std::string sk = keys.sk->toString();

    std::cout << "Public key for HTML: " << pk << std::endl;

    std::cout << "New zapros from client to registrate blinded_msg\n";
    std::string blinded_msg = "STUB_BLINDED_candidate_A";

    std::string blind_sig = BlindSign(sk, blinded_msg);

    std::cout << "Blind signature: " << blind_sig << std::endl;

    std::cout << "Voice verification\n";

    std::string msg = "candidate_A";
    std::string sig = "STUB_SIGNATURE_FINAL";

    bool valid = Verify(pk, msg, sig);
    std::cout << "Vote valid: " << valid << std::endl;
}