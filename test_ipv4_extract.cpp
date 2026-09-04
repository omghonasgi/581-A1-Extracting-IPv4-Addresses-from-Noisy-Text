// test_ipv4_extract.cpp
//
// Test driver for extractIPv4 in ipv4_extract.cpp. Runs a large table of
// inputs covering valid cases and edge cases (leading zeros, out-of-range
// octets/ports, malformed structure, garbage separators, etc.) and prints a
// pass/fail table.
//
// Build:   g++ -std=c++17 -Wall -Wextra -o test_ipv4_extract test_ipv4_extract.cpp
// Run:     ./test_ipv4_extract

#define IPV4_EXTRACT_NO_MAIN
#include "ipv4_extract.cpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct TestCase {
    const char* category;
    const char* label;
    const char* input;
    bool        expectMatch;   // true => expect a successful extraction
    const char* expectedIP;    // e.g. "192.168.1.1"; unused when expectMatch=false
    int         expectedPort;  // -1 for "no port"; unused when expectMatch=false
};

static std::string formatIP(unsigned long address) {
    unsigned long a = (address >> 24) & 0xFFUL;
    unsigned long b = (address >> 16) & 0xFFUL;
    unsigned long c = (address >>  8) & 0xFFUL;
    unsigned long d =  address        & 0xFFUL;
    std::ostringstream os;
    os << a << '.' << b << '.' << c << '.' << d;
    return os.str();
}

static std::string describe(bool matched, const std::string& ip, int port) {
    if (!matched) return "REJECT";
    std::ostringstream os;
    os << ip;
    if (port >= 0) os << ':' << port;
    return os.str();
}

int main() {
    const std::vector<TestCase> tests = {
        // --- basic valid --------------------------------------------------
        {"valid",         "basic dotted-quad",                "192.168.1.1",                    true,  "192.168.1.1",     -1},
        {"valid",         "all zeros",                        "0.0.0.0",                        true,  "0.0.0.0",         -1},
        {"valid",         "broadcast",                        "255.255.255.255",                true,  "255.255.255.255", -1},
        {"valid",         "with port",                        "10.0.0.255:8080",                true,  "10.0.0.255",      8080},
        {"valid",         "port 0 (bare zero allowed)",       "10.0.0.1:0",                     true,  "10.0.0.1",        0},
        {"valid",         "port max 65535",                   "10.0.0.1:65535",                 true,  "10.0.0.1",        65535},
        {"valid",         "embedded in prose",                "connecting to 192.168.1.1 now",  true,  "192.168.1.1",     -1},
        {"valid",         "prefix+suffix junk with port",     "server=10.0.0.255:8080end",      true,  "10.0.0.255",      8080},
        {"valid",         "second token is the valid one",    "bad.ip and then 8.8.8.8 later",  true,  "8.8.8.8",         -1},
        {"valid",         "bare-zero octet ok",               "0.10.20.30",                     true,  "0.10.20.30",      -1},
        {"valid",         "tab separator, valid follows",     "junk\t192.168.1.1",              true,  "192.168.1.1",     -1},
        {"valid",         "first-token invalid, second wins", "1.2.3.999 8.8.8.8",              true,  "8.8.8.8",         -1},

        // --- leading zeros in octets -------------------------------------
        {"leading zero",  "092 (first octet)",                "092.168.1.1",                    false, "",                -1},
        {"leading zero",  "01 (first octet)",                 "01.2.3.4",                       false, "",                -1},
        {"leading zero",  "001 (last octet)",                 "192.168.1.001",                  false, "",                -1},
        {"leading zero",  "00 (middle octet)",                "10.00.0.1",                      false, "",                -1},
        {"leading zero",  "01 (third octet)",                 "192.168.01.1",                   false, "",                -1},

        // --- leading zeros / invalid port --------------------------------
        {"port",          "leading zero 080",                 "10.0.0.1:080",                   false, "",                -1},
        {"port",          "leading zero 00",                  "10.0.0.1:00",                    false, "",                -1},
        {"port",          "leading zero 007",                 "10.0.0.1:007",                   false, "",                -1},
        {"port",          "out of range 65536",               "10.0.0.1:65536",                 false, "",                -1},
        {"port",          "out of range 99999",               "10.0.0.1:99999",                 false, "",                -1},
        {"port",          "6 digits (too many)",              "10.0.0.1:100000",                false, "",                -1},
        {"port",          "empty (trailing colon)",           "10.0.0.1:",                      false, "",                -1},
        {"port",          "double colon",                     "10.0.0.1:80:9",                  false, "",                -1},
        // Port digits must be *continuous* in the source: 'a' interrupts the
        // port digits and a further digit '0' follows before a clean break,
        // so the whole match is rejected (rather than accepting port 8).
        {"port",          "letter interrupts port digits",    "10.0.0.1:8a0",                   false, "",                -1},
        // Trailing all-alpha garbage after a complete port is fine.
        {"port",          "trailing alpha garbage ok",        "10.0.0.255:8080end",             true,  "10.0.0.255",      8080},
        // Same idea, but the trailer contains a digit AFTER letters -> reject.
        {"port",          "letters then digits after port",   "10.0.0.1:80abc42",               false, "",                -1},
        // Letters only after the port digits: still fine (like "8080end").
        {"port",          "letters only after port",          "10.0.0.1:80abc",                 true,  "10.0.0.1",        80},
        // A comma between port digits is still an interruption: only
        // whitespace (or end-of-string) is a clean boundary.
        {"port",          "comma between port digits",        "10.0.0.1:80,42",                 false, "",                -1},
        // Comma after the port with NO further digits is fine.
        {"port",          "comma then non-digits",            "10.0.0.1:80,end",                true,  "10.0.0.1",        80},
        // Whitespace IS a clean boundary: '42' after a space is unrelated.
        {"port",          "space boundary before more digits","10.0.0.1:80 stuff 42",           true,  "10.0.0.1",        80},
        // Tab is whitespace too.
        {"port",          "tab boundary before more digits",  "10.0.0.1:80\tstuff 42",          true,  "10.0.0.1",        80},

        // --- octets out of range -----------------------------------------
        {"octet range",   "256 (first)",                      "256.1.1.1",                      false, "",                -1},
        {"octet range",   "300 (second)",                     "1.300.1.1",                      false, "",                -1},
        {"octet range",   "999 (fourth)",                     "1.2.3.999",                      false, "",                -1},
        {"octet range",   "4-digit octet",                    "1.2.3.1000",                     false, "",                -1},

        // --- dots in the wrong places ------------------------------------
        // '.' is a token char, so the whole ".1.2.3.4" is one token starting
        // with '.', which parseToken rejects. No other candidates exist.
        {"structure",     "leading dot",                      ".1.2.3.4",                       false, "",                -1},
        {"structure",     "trailing dot",                     "1.2.3.4.",                       false, "",                -1},
        {"structure",     "double dot",                       "1..2.3.4",                       false, "",                -1},
        {"structure",     "only three octets",                "1.2.3",                          false, "",                -1},
        {"structure",     "five octets",                      "1.2.3.4.5",                      false, "",                -1},
        {"structure",     "just dots",                        "....",                           false, "",                -1},

        // --- colons in the wrong places ----------------------------------
        {"structure",     "leading colon",                    ":10.0.0.1",                      false, "",                -1},
        {"structure",     "colon in place of dot",            "10:0.0.1",                       false, "",                -1},
        {"structure",     "colon mid-address",                "10.0.0:1.2",                     false, "",                -1},

        // --- garbage / invalid characters --------------------------------
        {"garbage",       "letter inside octet splits token", "1.2.3a.4",                       false, "",                -1},
        {"garbage",       "space inside address",             "1.2. 3.4",                       false, "",                -1},
        {"garbage",       "hyphen splits, later half wins",   "1.2-3.4.5.6",                    true,  "3.4.5.6",         -1},
        {"garbage",       "underscore separators",            "1_2_3_4",                        false, "",                -1},
        {"garbage",       "hex-like octets",                  "0xC0.0xA8.0x01.0x01",            false, "",                -1},

        // --- incomplete / empty ------------------------------------------
        {"empty",         "empty string",                     "",                               false, "",                -1},
        {"empty",         "digits only",                      "12345",                          false, "",                -1},
        {"empty",         "no digits at all",                 "hello world",                    false, "",                -1},
    };

    // Column widths, computed from the data so the table stays aligned.
    size_t catW = 8, lblW = 5, inW = 5, expW = 8;
    for (const auto& t : tests) {
        catW = std::max(catW, std::string(t.category).size());
        lblW = std::max(lblW, std::string(t.label).size());
        inW  = std::max(inW,  std::string(t.input).size());
        expW = std::max(expW, describe(t.expectMatch, t.expectedIP, t.expectedPort).size());
    }

    const size_t resW = 8; // width of the "RESULT" column
    std::cout << std::left
              << std::setw(resW)   << "RESULT"
              << std::setw(catW+2) << "CATEGORY"
              << std::setw(lblW+2) << "LABEL"
              << std::setw(inW+2)  << "INPUT"
              << std::setw(expW+2) << "EXPECTED"
              << "ACTUAL\n";
    std::cout << std::string(resW + catW+2 + lblW+2 + inW+2 + expW+2 + 6, '-') << '\n';

    int passes = 0, fails = 0;
    for (const auto& t : tests) {
        unsigned long address = 0;
        int port = -1;
        bool matched = extractIPv4(t.input, address, port);
        std::string actualIP   = matched ? formatIP(address) : "";
        std::string actualStr  = describe(matched, actualIP, port);
        std::string expectStr  = describe(t.expectMatch, t.expectedIP, t.expectedPort);

        bool ok = false;
        if (matched == t.expectMatch) {
            if (!matched) ok = true;
            else ok = (actualIP == t.expectedIP) && (port == t.expectedPort);
        }

        if (ok) ++passes; else ++fails;

        std::cout << std::left
                  << std::setw(resW)   << (ok ? "OK" : "FAIL")
                  << std::setw(catW+2) << t.category
                  << std::setw(lblW+2) << t.label
                  << std::setw(inW+2)  << t.input
                  << std::setw(expW+2) << expectStr
                  << actualStr << '\n';
    }

    std::cout << '\n'
              << "Summary: " << passes << " passed, "
              << fails << " failed, "
              << tests.size() << " total\n";
    return fails == 0 ? 0 : 1;
}
