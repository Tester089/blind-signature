#define BLIND_SIG_IMPLEMENTATION
#include "core.h"
#include <iostream>



void client_example() {

    std::string pk;
    std::cin >> pk;

    std::string msg = "Candidate_A";

    std::string blind_result = Blind(pk, msg);
    std::string blinded_msg = blind_result.substr(0, blind_result.find(':'));
    std::string inv = blind_result.substr(blind_result.find(':') + 1);

    std::cout << "Blinded: " << blinded_msg << "\n";
    std::cout << "Inv: " << inv << "\n";


    std::cout << "Sending blinded_msg to server...\n";
    std::cout << "Enter blind_sig:";
    std::string blind_sig;
    std::cin >> blind_sig;

    std::string sig = Finalize(pk, msg, blind_sig, inv);

    bool ok = Verify(pk, msg, sig);
    if (ok) {std::cout << "Nice!";} else {std::cout << "We have some problems...";}
}


void server_example() {
    std::string keys = KeyGen();
    std::string pk = keys.substr(0, keys.find(':'));
    std::string sk = keys.substr(keys.find(':') + 1);


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
