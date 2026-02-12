#ifndef BLIND_SIGNATURE_CORE_H
#define BLIND_SIGNATURE_CORE_H

#include <string>


// ============= FOR EVERY SIDE ====================
std::string KeyGen();
bool Verify(const std::string& pk, const std::string& msg, const std::string& sig);


// ============= FOR CLIENT SIDE ===================
std::string Blind(const std::string& pk, const std::string& msg);
std::string Finalize(const std::string& pk, const std::string& msg, const std::string& blind_sig, const std::string& inv);


// ============= FOR SERVER SIDE ===================
std::string BlindSign(const std::string& sk, const std::string& blinded_msg);


#endif



#ifdef BLIND_SIG_IMPLEMENTATION

std::string KeyGen() {
  return "STUB_PK:STUB_SK";
}

std::string Blind(const std::string& pk, const std::string& msg) {
  return "BLINDED_" + msg + ":INV42_BRATUHA";
}

std::string BlindSign(const std::string& sk, const std::string& blinded_msg) {
  return "BLIND_SIG_" + blinded_msg;
}

std::string Finalize(const std::string& pk, const std::string& msg, const std::string& blind_sig, const std::string& inv) {
  return "SIG_STUB";
}

bool Verify(const std::string& pk, const std::string& msg, const std::string& sig) {
  return true;
}

#endif