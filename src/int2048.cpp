#include "int2048.h"

namespace sjtu {

// Constructors
int2048::int2048() : negative(false) {
    digits.push_back(0);
}

int2048::int2048(long long x) : negative(false) {
    if (x < 0) {
        negative = true;
        x = -x;
    }
    if (x == 0) {
        digits.push_back(0);
    } else {
        while (x > 0) {
            digits.push_back(x % BASE);
            x /= BASE;
        }
    }
}

int2048::int2048(const std::string &s) : negative(false) {
    read(s);
}

int2048::int2048(const int2048 &other) : digits(other.digits), negative(other.negative) {
}

void int2048::normalize() {
    while (digits.size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
    if (digits.size() == 1 && digits[0] == 0) {
        negative = false;
    }
}

int int2048::compare_abs(const int2048 &other) const {
    if (digits.size() != other.digits.size()) {
        return digits.size() < other.digits.size() ? -1 : 1;
    }
    for (int i = (int)digits.size() - 1; i >= 0; i--) {
        if (digits[i] != other.digits[i]) {
            return digits[i] < other.digits[i] ? -1 : 1;
        }
    }
    return 0;
}

int2048 int2048::add_abs(const int2048 &a, const int2048 &b) {
    int2048 r;
    r.digits.resize(std::max(a.digits.size(), b.digits.size()) + 1, 0);
    
    long long carry = 0;
    for (size_t i = 0; i < r.digits.size(); i++) {
        long long sum = carry;
        if (i < a.digits.size()) sum += a.digits[i];
        if (i < b.digits.size()) sum += b.digits[i];
        r.digits[i] = sum % BASE;
        carry = sum / BASE;
    }
    
    r.normalize();
    return r;
}

int2048 int2048::sub_abs(const int2048 &a, const int2048 &b) {
    int2048 r;
    r.digits.resize(a.digits.size(), 0);
    
    long long borrow = 0;
    for (size_t i = 0; i < a.digits.size(); i++) {
        long long diff = a.digits[i] - borrow - (i < b.digits.size() ? b.digits[i] : 0);
        if (diff < 0) {
            diff += BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        r.digits[i] = diff;
    }
    
    r.normalize();
    return r;
}

int2048 int2048::mul_abs_simple(const int2048 &a, const int2048 &b) {
    int2048 r;
    r.digits.assign(a.digits.size() + b.digits.size(), 0);
    
    for (size_t i = 0; i < a.digits.size(); i++) {
        long long carry = 0;
        for (size_t j = 0; j < b.digits.size() || carry; j++) {
            long long cur = r.digits[i + j] + carry;
            if (j < b.digits.size()) cur += a.digits[i] * b.digits[j];
            r.digits[i + j] = cur % BASE;
            carry = cur / BASE;
        }
    }
    
    r.normalize();
    return r;
}

// NTT-based multiplication for large numbers
namespace {
    const long long NTT_MOD = 998244353;
    const long long NTT_G = 3;
    
    long long mod_pow(long long a, long long b, long long m) {
        long long res = 1;
        a %= m;
        while (b > 0) {
            if (b & 1) res = res * a % m;
            a = a * a % m;
            b >>= 1;
        }
        return res;
    }
    
    void ntt(std::vector<long long>& a, bool inv) {
        int n = a.size();
        for (int i = 1, j = 0; i < n; i++) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap(a[i], a[j]);
        }
        
        for (int len = 2; len <= n; len <<= 1) {
            long long w = inv ? mod_pow(NTT_G, NTT_MOD - 1 - (NTT_MOD - 1) / len, NTT_MOD)
                              : mod_pow(NTT_G, (NTT_MOD - 1) / len, NTT_MOD);
            for (int i = 0; i < n; i += len) {
                long long wn = 1;
                for (int j = 0; j < len / 2; j++) {
                    long long u = a[i + j];
                    long long v = a[i + j + len / 2] * wn % NTT_MOD;
                    a[i + j] = (u + v) % NTT_MOD;
                    a[i + j + len / 2] = (u - v + NTT_MOD) % NTT_MOD;
                    wn = wn * w % NTT_MOD;
                }
            }
        }
        
        if (inv) {
            long long n_inv = mod_pow(n, NTT_MOD - 2, NTT_MOD);
            for (int i = 0; i < n; i++) a[i] = a[i] * n_inv % NTT_MOD;
        }
    }
    
    std::vector<long long> convolution(const std::vector<long long>& a, const std::vector<long long>& b) {
        int n = 1;
        while (n < (int)(a.size() + b.size())) n <<= 1;
        
        std::vector<long long> fa(a.begin(), a.end()), fb(b.begin(), b.end());
        fa.resize(n);
        fb.resize(n);
        
        ntt(fa, false);
        ntt(fb, false);
        for (int i = 0; i < n; i++) fa[i] = fa[i] * fb[i] % NTT_MOD;
        ntt(fa, true);
        
        return fa;
    }
}

int2048 int2048::mul_abs(const int2048 &a, const int2048 &b) {
    // Use simple multiplication for all cases - it's correct and reliable
    // For very large numbers, this may be slow but gives correct results
    return mul_abs_simple(a, b);
}

int2048 int2048::div_abs(const int2048 &a, const int2048 &b, int2048 &rem) {
    // Handle division by zero (undefined behavior)
    if (b.digits.size() == 1 && b.digits[0] == 0) {
        rem = a;
        rem.negative = false;
        return int2048();
    }
    
    int cmp = a.compare_abs(b);
    if (cmp < 0) {
        rem = a;
        rem.negative = false;
        return int2048();
    }
    if (cmp == 0) {
        rem.digits.assign(1, 0);
        rem.negative = false;
        int2048 one;
        one.digits[0] = 1;
        return one;
    }
    
    // Long division
    int2048 quot;
    quot.digits.assign(a.digits.size(), 0);
    rem.digits.clear();
    rem.negative = false;
    
    for (int i = (int)a.digits.size() - 1; i >= 0; i--) {
        // Shift remainder left and add next digit
        rem.digits.insert(rem.digits.begin(), a.digits[i]);
        rem.normalize();
        
        // Binary search for the quotient digit
        long long lo = 0, hi = BASE - 1;
        while (lo < hi) {
            long long mid = (lo + hi + 1) / 2;
            
            // Compute mid * b for comparison
            int2048 prod;
            prod.digits.assign(b.digits.size() + 1, 0);
            long long carry = 0;
            for (size_t j = 0; j < b.digits.size(); j++) {
                long long p = mid * b.digits[j] + carry;
                prod.digits[j] = p % BASE;
                carry = p / BASE;
            }
            prod.digits[b.digits.size()] = carry;
            prod.normalize();
            
            if (prod.compare_abs(rem) <= 0) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        
        quot.digits[i] = lo;
        
        // Subtract lo * b from remainder
        if (lo > 0) {
            long long borrow = 0;
            for (size_t j = 0; j < rem.digits.size(); j++) {
                long long val = rem.digits[j] - borrow - (j < b.digits.size() ? lo * b.digits[j] : 0);
                if (val < 0) {
                    long long c = (-val + BASE - 1) / BASE;
                    val += c * BASE;
                    borrow = c;
                } else {
                    borrow = 0;
                }
                rem.digits[j] = val;
            }
            rem.normalize();
        }
    }
    
    quot.normalize();
    rem.normalize();
    return quot;
}

void int2048::read(const std::string &s) {
    digits.clear();
    negative = false;
    
    size_t start = 0;
    if (!s.empty() && s[0] == '-') {
        negative = true;
        start = 1;
    } else if (!s.empty() && s[0] == '+') {
        start = 1;
    }
    
    // Skip leading zeros
    while (start < s.size() - 1 && s[start] == '0') {
        start++;
    }
    
    int len = s.size() - start;
    if (len == 0) {
        digits.push_back(0);
        normalize();
        return;
    }
    
    int num_digits = (len + BASE_DIGITS - 1) / BASE_DIGITS;
    digits.resize(num_digits);
    
    for (int i = 0; i < num_digits; i++) {
        int end = len - i * BASE_DIGITS;
        int begin = std::max(0, end - BASE_DIGITS);
        long long val = 0;
        for (int j = begin; j < end; j++) {
            val = val * 10 + (s[start + j] - '0');
        }
        digits[i] = val;
    }
    
    normalize();
}

void int2048::print() {
    if (negative) {
        putchar('-');
    }
    printf("%lld", digits.back());
    for (int i = (int)digits.size() - 2; i >= 0; i--) {
        printf("%09lld", digits[i]);
    }
}

int2048& int2048::add(const int2048 &other) {
    if (negative == other.negative) {
        int2048 r = add_abs(*this, other);
        r.negative = negative;
        digits.swap(r.digits);
        negative = r.negative;
    } else {
        int cmp = compare_abs(other);
        if (cmp >= 0) {
            int2048 r = sub_abs(*this, other);
            r.negative = negative;
            digits.swap(r.digits);
            negative = r.negative;
        } else {
            int2048 r = sub_abs(other, *this);
            r.negative = other.negative;
            digits.swap(r.digits);
            negative = r.negative;
        }
    }
    normalize();
    return *this;
}

int2048 add(int2048 a, const int2048 &b) {
    return a.add(b);
}

int2048& int2048::minus(const int2048 &other) {
    if (negative != other.negative) {
        int2048 r = add_abs(*this, other);
        r.negative = negative;
        digits.swap(r.digits);
        negative = r.negative;
    } else {
        int cmp = compare_abs(other);
        if (cmp >= 0) {
            int2048 r = sub_abs(*this, other);
            r.negative = negative;
            digits.swap(r.digits);
            negative = r.negative;
        } else {
            int2048 r = sub_abs(other, *this);
            r.negative = !negative;
            digits.swap(r.digits);
            negative = r.negative;
        }
    }
    normalize();
    return *this;
}

int2048 minus(int2048 a, const int2048 &b) {
    return a.minus(b);
}

// Integer2 operators
int2048 int2048::operator+() const {
    return *this;
}

int2048 int2048::operator-() const {
    int2048 r = *this;
    if (r.digits.size() > 1 || r.digits[0] != 0) {
        r.negative = !r.negative;
    }
    return r;
}

int2048& int2048::operator=(const int2048 &other) {
    if (this != &other) {
        digits = other.digits;
        negative = other.negative;
    }
    return *this;
}

int2048& int2048::operator+=(const int2048 &other) {
    return add(other);
}

int2048 operator+(int2048 a, const int2048 &b) {
    return a += b;
}

int2048& int2048::operator-=(const int2048 &other) {
    return minus(other);
}

int2048 operator-(int2048 a, const int2048 &b) {
    return a -= b;
}

int2048& int2048::operator*=(const int2048 &other) {
    int2048 r = mul_abs(*this, other);
    r.negative = (negative != other.negative) && (r.digits.size() > 1 || r.digits[0] != 0);
    digits.swap(r.digits);
    negative = r.negative;
    return *this;
}

int2048 operator*(int2048 a, const int2048 &b) {
    return a *= b;
}

int2048& int2048::operator/=(const int2048 &other) {
    int2048 rem;
    int2048 quot = div_abs(*this, other, rem);
    
    bool signs_differ = (negative != other.negative);
    bool has_rem = (rem.digits.size() > 1 || rem.digits[0] != 0);
    
    // Floor division: round toward negative infinity
    // If signs differ and remainder != 0, the floor is -(Q+1) where Q = |quotient|
    if (signs_differ && has_rem) {
        // Add 1 to quotient, then negate (result is -(Q+1))
        long long carry = 1;
        for (size_t i = 0; i < quot.digits.size() && carry; i++) {
            quot.digits[i] += carry;
            if (quot.digits[i] >= BASE) {
                quot.digits[i] -= BASE;
                carry = 1;
            } else {
                carry = 0;
            }
        }
        if (carry) {
            quot.digits.push_back(1);
        }
        // quot is now Q+1, we want -(Q+1)
        quot.negative = true;
    } else {
        // No adjustment needed, just set sign
        quot.negative = signs_differ && (quot.digits.size() > 1 || quot.digits[0] != 0);
    }
    
    digits.swap(quot.digits);
    negative = quot.negative;
    return *this;
}

int2048 operator/(int2048 a, const int2048 &b) {
    return a /= b;
}

int2048& int2048::operator%=(const int2048 &other) {
    int2048 rem;
    div_abs(*this, other, rem);
    
    // Python modulo: x % y = x - floor(x/y) * y
    // If signs differ and remainder != 0, adjust: rem = |b| - rem
    if (negative != other.negative && (rem.digits.size() > 1 || rem.digits[0] != 0)) {
        rem = sub_abs(other, rem);
    }
    
    // Remainder has sign of divisor
    rem.negative = other.negative && (rem.digits.size() > 1 || rem.digits[0] != 0);
    if (!other.negative) rem.negative = false;
    
    digits.swap(rem.digits);
    negative = rem.negative;
    return *this;
}

int2048 operator%(int2048 a, const int2048 &b) {
    return a %= b;
}

std::istream& operator>>(std::istream &is, int2048 &obj) {
    std::string s;
    is >> s;
    obj.read(s);
    return is;
}

std::ostream& operator<<(std::ostream &os, const int2048 &obj) {
    if (obj.negative) os << '-';
    os << obj.digits.back();
    for (int i = (int)obj.digits.size() - 2; i >= 0; i--) {
        os << std::setw(9) << std::setfill('0') << obj.digits[i];
    }
    return os;
}

bool operator==(const int2048 &a, const int2048 &b) {
    return a.negative == b.negative && a.digits == b.digits;
}

bool operator!=(const int2048 &a, const int2048 &b) {
    return !(a == b);
}

bool operator<(const int2048 &a, const int2048 &b) {
    if (a.negative != b.negative) return a.negative;
    int cmp = a.compare_abs(b);
    return a.negative ? cmp > 0 : cmp < 0;
}

bool operator>(const int2048 &a, const int2048 &b) {
    return b < a;
}

bool operator<=(const int2048 &a, const int2048 &b) {
    return !(a > b);
}

bool operator>=(const int2048 &a, const int2048 &b) {
    return !(a < b);
}

} // namespace sjtu
