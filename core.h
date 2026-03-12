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

const unsigned long long BASA = 4294967296u;
const unsigned int MAX_NUMBER = 4294967296 - 1;
const unsigned int HIGHEST_BIT = 1u << 31;

struct BigInt {
    std::vector<unsigned int> d;

    BigInt() {
        d.push_back(0);
    }

    BigInt(unsigned long long v) {
        d.push_back(v & MAX_NUMBER);
        unsigned int hi = v >> 32;
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


// Can be updated to Knuth Algorithm D, but for our presentation it overkill (IMHO).
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
            r_top = ((unsigned long long)r.d[rsz - 1] << 32) | r.d[rsz - 2];
        } else {
            r_top = r.d[rsz - 1];
        }

        unsigned long long qi = 0;
        if (b_top > 0) {
            qi = r_top / (b_top + 1);
        } else {
            qi = MAX_NUMBER;
        }
        if (qi > MAX_NUMBER) qi = MAX_NUMBER;

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
            exp.d[i] = (exp.d[i] >> 1) | (exp.d[i + 1] << 31);
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
        ny = add(x1, qy1);
        nyn = x1n;
    } else {
        int c = cmp(x1, qy1);
        if (c >= 0) {
            ny = sub(x1, qy1);
            nyn = x1n;
        } else {
            ny = sub(qy1, x1);
            nyn = !x1n;
        }
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
        oss << std::hex << std::setw(8) << std::setfill('0') << n.d[i];
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
    while (padded.size() % 8 != 0) {
        padded = "0" + padded;
    }
    for (int i = (int)padded.size(); i > 0; i -= 8) {
        std::string chunk = padded.substr(i - 8, 8);
        r.d.push_back((unsigned int)std::stoul(chunk, nullptr, 16));
    }
    r.trim();
    return r;
}

inline BigInt random_bigint(size_t bits, std::mt19937_64& rng) {
    BigInt r;
    size_t chunks = (bits + 31) / 32;
    r.d.resize(chunks, 0);
    for (auto& chunk : r.d) {
        chunk = (unsigned int)(rng() % BASA);
    }
    size_t top_bits = bits % 32;
    if (top_bits != 0) {
        r.d.back() = r.d.back() % (1u << top_bits);
    }
    if (top_bits != 0) {
        r.d.back() = r.d.back() | (1u << (top_bits - 1));
    } else {
        r.d.back() = r.d.back() | (HIGHEST_BIT);
    }
    r.trim();
    return r;
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