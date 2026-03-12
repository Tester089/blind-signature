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


struct BigInt {
    std::vector<unsigned int> d;

    BigInt() {
        d.push_back(0);
    }

    BigInt(unsigned long long v) {
        d.push_back(v & 4294967295u);
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
    res.d.assign(maxsz, 0);
    unsigned long long carry = 0;
    for (size_t i = 0; i < res.d.size(); i++) {
        unsigned long long s = carry;
        if (i < a.d.size()) s += a.d[i];
        if (i < b.d.size()) s += b.d[i];
        res.d[i] = (unsigned int)(s % 4294967296u);
        carry = s / 4294967296u;
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
            diff += 4294967296ll;
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
            res.d[i + j] = (unsigned int)(cur % 4294967296u);
            carry = cur / 4294967296u;
        }
        res.d[i + b.d.size()] += (unsigned int)carry;
    }
    res.trim();
    return res;
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

