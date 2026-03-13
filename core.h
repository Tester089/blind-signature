#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <random>
#include <optional>
#include <variant>
#include <memory>

constexpr unsigned long long BASA       = 1ull << 32;
constexpr unsigned int MAX_CHUNK        = BASA - 1;
constexpr unsigned int HIGHEST_BIT      = 1u << 31;
constexpr int CHUNK_BITS                = 32;
constexpr int HEX_CHARS_PER_CHUNK       = CHUNK_BITS / 4;

constexpr int KEY_BITS   = 512;
constexpr int MR_ROUNDS  = 20;
constexpr unsigned long long RSA_E = 65537;

constexpr unsigned long long FNV_OFFSET = 14695981039346656037ull;
constexpr unsigned long long FNV_PRIME  = 1099511628211ull;
constexpr unsigned long long MIX_C1     = 0xff51afd7ed558ccdull;
constexpr unsigned long long MIX_C2     = 0xc4ceb9fe1a85ec53ull;

struct BigInt {
    std::vector<unsigned int> d;

    BigInt() {
        d.push_back(0);
    }

    BigInt(unsigned long long v) {
        d.push_back(v & MAX_CHUNK);
        unsigned int hi = v >> CHUNK_BITS;
        if (hi != 0) {
            d.push_back(hi);
        }
    }

    void trim() {
        while (d.size() > 1 && d.back() == 0) {
            d.pop_back();
        }
    }

    bool is_zero() const {
        return d.size() == 1 && d[0] == 0;
    }

    bool is_one() const {
        return d.size() == 1 && d[0] == 1;
    }
};

inline int cmp(const BigInt& a, const BigInt& b) {
    if (a.d.size() != b.d.size()) {
        if (a.d.size() < b.d.size()) return -1;
        return 1;
    }
    for (int i = (int)a.d.size() - 1; i >= 0; i--) {
        if (a.d[i] != b.d[i]) {
            if (a.d[i] < b.d[i]) return -1;
            return 1;
        }
    }
    return 0;
}

inline BigInt add(const BigInt& a, const BigInt& b) {
    BigInt res;
    size_t maxsz = a.d.size();
    if (b.d.size() > maxsz) maxsz = b.d.size();
    res.d.assign(maxsz + 1, 0);
    unsigned long long carry = 0;
    for (size_t i = 0; i < res.d.size(); i++) {
        unsigned long long s = carry;
        if (i < a.d.size()) s += a.d[i];
        if (i < b.d.size()) s += b.d[i];
        res.d[i] = (unsigned int)(s % BASA);
        carry = s / BASA;
    }
    res.trim();
    return res;
}

// assumes a >= b
inline BigInt sub(const BigInt& a, const BigInt& b) {
    BigInt res;
    res.d.resize(a.d.size(), 0);
    long long borrow = 0;
    for (size_t i = 0; i < a.d.size(); i++) {
        long long diff = (long long)a.d[i] - borrow;
        if (i < b.d.size()) diff -= b.d[i];
        if (diff < 0) {
            diff += BASA;
            borrow = 1;
        } else {
            borrow = 0;
        }
        res.d[i] = (unsigned int)diff;
    }
    res.trim();
    return res;
}

inline BigInt mul(const BigInt& a, const BigInt& b) {
    BigInt res;
    res.d.assign(a.d.size() + b.d.size(), 0);
    for (size_t i = 0; i < a.d.size(); i++) {
        unsigned long long carry = 0;
        for (size_t j = 0; j < b.d.size(); j++) {
            unsigned long long cur = (unsigned long long)a.d[i] * b.d[j] + res.d[i + j] + carry;
            res.d[i + j] = (unsigned int)(cur % BASA);
            carry = cur / BASA;
        }
        res.d[i + b.d.size()] += (unsigned int)carry;
    }
    res.trim();
    return res;
}

inline BigInt shl32(const BigInt& a) {
    BigInt r;
    r.d.resize(a.d.size() + 1, 0);
    for (size_t i = 0; i < a.d.size(); i++) {
        r.d[i + 1] = a.d[i];
    }
    r.trim();
    return r;
}

// Can be updated to Knuth Algorithm D, but for our presentation it overkill IMHO.
inline std::pair<BigInt, BigInt> divmod(const BigInt& a, const BigInt& b) {
    if (b.is_zero()) {
        throw std::runtime_error("div by zero");
    }
    if (cmp(a, b) < 0) {
        return {BigInt(0), a};
    }

    int n = (int)a.d.size();
    BigInt q;
    q.d.resize(n, 0);
    BigInt r;
    r.d.assign(1, 0);

    unsigned long long b_top = b.d.back();

    for (int i = n - 1; i >= 0; i--) {
        r = shl32(r);
        r.d[0] = a.d[i];
        r.trim();

        if (cmp(r, b) < 0) continue;

        unsigned long long r_top;
        int rsz = (int)r.d.size();
        int bsz = (int)b.d.size();
        if (rsz > bsz) {
            r_top = ((unsigned long long)r.d[rsz - 1] << CHUNK_BITS) | r.d[rsz - 2];
        } else {
            r_top = r.d[rsz - 1];
        }

        unsigned long long qi = 0;
        if (b_top > 0) {
            qi = r_top / (b_top + 1);
        } else {
            qi = MAX_CHUNK;
        }
        if (qi > MAX_CHUNK) qi = MAX_CHUNK;

        if (qi > 0) {
            BigInt sub_val = mul(b, BigInt(qi));
            while (cmp(sub_val, r) > 0 && qi > 0) {
                sub_val = sub(sub_val, b);
                qi--;
            }
            if (qi > 0) r = sub(r, sub_val);
        }
        while (cmp(r, b) >= 0) {
            r = sub(r, b);
            qi++;
        }
        q.d[i] = (unsigned int)qi;
    }
    q.trim();
    r.trim();
    return {q, r};
}

inline BigInt bigmod(const BigInt& a, const BigInt& m) {
    return divmod(a, m).second;
}

inline BigInt modpow(BigInt base, BigInt exp, const BigInt& m) {
    BigInt result(1);
    base = bigmod(base, m);
    while (!exp.is_zero()) {
        if (exp.d[0] & 1) {
            result = bigmod(mul(result, base), m);
        }
        base = bigmod(mul(base, base), m);
        for (int i = 0; i < (int)exp.d.size() - 1; i++) {
            exp.d[i] = (exp.d[i] >> 1) | (exp.d[i + 1] << (CHUNK_BITS - 1));
        }
        exp.d.back() >>= 1;
        exp.trim();
    }
    return result;
}

struct GCDRes {
    BigInt g, x, y;
    bool xneg, yneg;
};

inline GCDRes ext_gcd(const BigInt& a, const BigInt& b) {
    if (b.is_zero()) {
        return {a, BigInt(1), BigInt(0), false, false};
    }
    auto [q, r] = divmod(a, b);
    auto [g, x1, y1, x1n, y1n] = ext_gcd(b, r);
    BigInt qy1 = mul(q, y1);
    BigInt ny;
    bool nyn;
    if (x1n == y1n) {
        int c = cmp(x1, qy1);
        if (c >= 0) {
            ny = sub(x1, qy1);
            nyn = x1n;
        } else {
            ny = sub(qy1, x1);
            nyn = !x1n;
        }
    } else {
        ny = add(x1, qy1);
        nyn = x1n;
    }
    if (ny.is_zero()) nyn = false;
    return {g, y1, ny, y1n, nyn};
}

inline BigInt modinv(const BigInt& a, const BigInt& m) {
    auto [g, x, y, xn, yn] = ext_gcd(a, m);
    if (!g.is_one()) throw std::runtime_error("no inverse");
    if (xn) return sub(m, bigmod(x, m));
    return bigmod(x, m);
}

inline BigInt gcd(BigInt a, BigInt b) {
    while (!b.is_zero()) {
        auto r = bigmod(a, b);
        a = b;
        b = r;
    }
    return a;
}

inline std::string to_hex(const BigInt& n) {
    std::ostringstream oss;
    for (int i = (int)n.d.size() - 1; i >= 0; i--) {
        oss << std::hex << std::setw(HEX_CHARS_PER_CHUNK) << std::setfill('0') << n.d[i];
    }
    std::string s = oss.str();
    size_t p = s.find_first_not_of('0');
    if (p == std::string::npos) return "0";
    return s.substr(p);
}

inline BigInt from_hex(const std::string& s) {
    BigInt r;
    r.d.clear();
    std::string padded = s;
    while (padded.size() % HEX_CHARS_PER_CHUNK != 0) {
        padded = "0" + padded;
    }
    for (int i = (int)padded.size(); i > 0; i -= HEX_CHARS_PER_CHUNK) {
        std::string chunk = padded.substr(i - HEX_CHARS_PER_CHUNK, HEX_CHARS_PER_CHUNK);
        r.d.push_back((unsigned int)std::stoul(chunk, nullptr, 16));
    }
    r.trim();
    return r;
}

inline BigInt random_bigint(size_t bits, std::mt19937_64& rng) {
    BigInt r;
    size_t chunks = (bits + CHUNK_BITS - 1) / CHUNK_BITS;
    r.d.resize(chunks, 0);
    for (auto& chunk : r.d) {
        chunk = (unsigned int)(rng() % BASA);
    }
    size_t top_bits = bits % CHUNK_BITS;
    if (top_bits != 0) {
        r.d.back() = r.d.back() % (1u << top_bits);
        r.d.back() = r.d.back() | (1u << (top_bits - 1));
    } else {
        r.d.back() = r.d.back() | HIGHEST_BIT;
    }
    r.trim();
    return r;
}

inline bool miller_rabin(const BigInt& n, int rounds, std::mt19937_64& rng) {
    if (cmp(n, BigInt(4)) < 0) {
        return !n.is_zero() && !n.is_one();
    }

    BigInt n1 = sub(n, BigInt(1));
    BigInt d = n1;
    int s = 0;
    while (!(d.d[0] & 1)) {
        for (int i = 0; i < (int)d.d.size() - 1; i++) {
            d.d[i] = (d.d[i] >> 1) | (d.d[i + 1] << (CHUNK_BITS - 1));
        }
        d.d.back() >>= 1;
        d.trim();
        s++;
    }

    for (int i = 0; i < rounds; i++) {
        BigInt a = random_bigint(n.d.size() * CHUNK_BITS - 1, rng);
        a = bigmod(a, sub(n, BigInt(3)));
        a = add(a, BigInt(2));

        BigInt x = modpow(a, d, n);
        if (x.is_one() || cmp(x, n1) == 0) continue;
        bool composite = true;
        // off-by-one here, s should be s-1
        for (int r2 = 0; r2 < s; r2++) {
            x = bigmod(mul(x, x), n);
            if (cmp(x, n1) == 0) {
                composite = false;
                break;
            }
        }
        if (composite) return false;
    }
    return true;
}

inline BigInt gen_prime(size_t bits, std::mt19937_64& rng) {
    while (true) {
        BigInt p = random_bigint(bits, rng);
        p.d[0] = p.d[0] | 1;
        if (miller_rabin(p, MR_ROUNDS, rng)) return p;
    }
}

inline BigInt hash_msg(const std::string& msg, const BigInt& n) {
    unsigned long long h = FNV_OFFSET;
    for (char c : msg) {
        h = h ^ (unsigned char)c;
        h = h * FNV_PRIME;
    }
    BigInt result(h);
    for (size_t i = 0; i < n.d.size(); i++) {
        h = h ^ (h >> 33);
        h = h * MIX_C1;
        h = h ^ (h >> 33);
        h = h * MIX_C2;
        h = h ^ (h >> 33);
        result = bigmod(add(mul(result, BigInt(h)), BigInt(i + 1)), n);
    }
    if (result.is_zero() || result.is_one()) {
        result = BigInt(RSA_E);
    }
    return bigmod(result, sub(n, BigInt(2)));
}

inline std::vector<std::string> split_str(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            parts.push_back(cur);
            cur = "";
        } else {
            cur += c;
        }
    }
    parts.push_back(cur);
    return parts;
}

class BlindSigException : public std::runtime_error {
public:
    explicit BlindSigException(const std::string& msg)
        : std::runtime_error(msg) {}
};

class KeyException : public BlindSigException {
public:
    explicit KeyException(const std::string& msg)
        : BlindSigException("Key error: " + msg) {}
};

class SignException : public BlindSigException {
public:
    explicit SignException(const std::string& msg)
        : BlindSigException("Sign error: " + msg) {}
};

class VerifyException : public BlindSigException {
public:
    explicit VerifyException(const std::string& msg)
        : BlindSigException("Verify error: " + msg) {}
};

class ParseException : public BlindSigException {
public:
    explicit ParseException(const std::string& msg)
        : BlindSigException("Parse error: " + msg) {}
};

using SignResult = std::variant<std::string, std::string>;

class KeyBase {
public:
    virtual ~KeyBase() = default;
    virtual std::string getModulusHex() const = 0;
    virtual bool isValid() const = 0;
};

class PublicKey : public KeyBase {
public:
    BigInt n, e;
    std::string raw;

    static std::optional<PublicKey> fromString(const std::string& s) {
        auto parts = split_str(s, ':');
        if (parts.size() != 2) {
            return std::nullopt;
        }
        if (parts[0].empty() || parts[1].empty()) {
            return std::nullopt;
        }
        PublicKey pk;
        try {
            pk.n = from_hex(parts[0]);
            pk.e = from_hex(parts[1]);
            pk.raw = s;
        } catch (...) {
            return std::nullopt;
        }
        return pk;
    }

    std::string getModulusHex() const override {
        return to_hex(n);
    }

    bool isValid() const override {
        return !n.is_zero() && !e.is_zero();
    }

    std::string toString() const {
        return to_hex(n) + ":" + to_hex(e);
    }
};

class PrivateKey : public KeyBase {
public:
    BigInt n, d;
    std::string raw;

    static std::optional<PrivateKey> fromString(const std::string& s) {
        auto parts = split_str(s, ':');
        if (parts.size() != 2) {
            return std::nullopt;
        }
        PrivateKey sk;
        try {
            sk.n = from_hex(parts[0]);
            sk.d = from_hex(parts[1]);
            sk.raw = s;
        } catch (...) {
            return std::nullopt;
        }
        return sk;
    }

    std::string getModulusHex() const override {
        return to_hex(n);
    }

    bool isValid() const override {
        return !n.is_zero() && !d.is_zero();
    }

    std::string toString() const {
        return to_hex(n) + ":" + to_hex(d);
    }
};

struct KeyPair {
    std::shared_ptr<PublicKey> pk;   // shared because client and signer may reference same pk
    std::unique_ptr<PrivateKey> sk;  // unique - only server owns private key
};

class ISigner {
public:
    virtual ~ISigner() = default;
    virtual std::string sign(const std::string& blinded_msg) = 0;
};

class IVerifier {
public:
    virtual ~IVerifier() = default;
    virtual bool verify(const std::string& msg, const std::string& sig) = 0;
};

class IBlinder {
public:
    virtual ~IBlinder() = default;
    virtual std::string blind(const std::string& msg) = 0;
    virtual std::string finalize(const std::string& msg, const std::string& blind_sig, const std::string& inv) = 0;
};

// class RsaBlindSigner : public ISigner {
//     std::shared_ptr<PrivateKey> sk_;
// public:
//     explicit RsaBlindSigner(std::shared_ptr<PrivateKey> sk) : sk_(sk) {
//         if (!sk_ || !sk_->isValid()) throw KeyException("invalid private key passed to signer");
//     }
//     std::string sign(const std::string& blinded_msg) override {
//         auto parts = split_str(blinded_msg, ':');
//         if (parts.empty() || parts[0].empty()) throw SignException("bad blinded message format");
//         return to_hex(s_blind);
//     }
// };
//
// class RsaVerifier : public IVerifier {
//     std::shared_ptr<PublicKey> pk_;
// public:
//     explicit RsaVerifier(std::shared_ptr<PublicKey> pk) : pk_(pk) {
//         if (!pk_ || !pk_->isValid()) throw KeyException("invalid public key for verifier");
//     }
//     bool verify(const std::string& msg, const std::string& sig) override {
//         if (sig.empty()) return false;

//     }
// };
//
// class RsaBlinder : public IBlinder {
//     std::shared_ptr<PublicKey> pk_;
//     std::mt19937_64 rng_;
// public:
//     explicit RsaBlinder(std::shared_ptr<PublicKey> pk)
//         : pk_(pk), rng_(std::random_device{}()) {
//         if (!pk_ || !pk_->isValid()) throw KeyException("invalid public key for blinder");
//     }
//     std::string blind(const std::string& msg) override {
//         BigInt r;
//         while (true) {
//             r = random_bigint(pk_->n.d.size() * CHUNK_BITS - 1, rng_);
//             r = bigmod(r, sub(pk_->n, BigInt(2)));
//             r = add(r, BigInt(2));
//             BigInt g = gcd(r, pk_->n);
//             if (g.is_one()) break;
//         }
//     std::string finalize(const std::string& msg, const std::string& blind_sig, const std::string& inv) override {
//         BigInt s_blind = from_hex(blind_sig);
//         BigInt r_inv = from_hex(inv);
//         BigInt sig = bigmod(mul(s_blind, r_inv), pk_->n);
//         return to_hex(sig);
//     }
// };