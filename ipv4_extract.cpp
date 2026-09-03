// ipv4_extract.cpp
//
// Reads lines of text from the user and extracts the first valid IPv4 address
// (with an optional port) embedded anywhere in the line.
//
// Tokenization rule: a "token" is a maximal run of characters where every
// character is a digit ('0'-'9'), a period ('.'), or a colon (':'). Any other
// character is treated as a separator and skipped. The first token that parses
// as a valid dotted-quad (each octet 0-255), optionally followed by ":<port>"
// (port 0-65535), is returned.
//
// All numeric parsing is done manually. None of these are used:
//   atoi, atol, atoll, strtol, strtoul, strtod, stoi, stol, stoul,
//   sscanf, inet_aton, inet_pton, inet_addr, or any regex library.

#include <iostream>
#include <string>

// --- Small character helpers (avoid <cctype> just to keep intent obvious) ---
static bool isDigitChar(char c) {
    return c >= '0' && c <= '9';
}

static bool isTokenChar(char c) {
    return isDigitChar(c) || c == '.' || c == ':';
}

// Attempts to parse a single token (a run of digits/'.'/':') as either
// "A.B.C.D" or "A.B.C.D:P". On success, fills octets[0..3] and *outPort
// (or -1 if no port was present) and returns true.
static bool parseToken(const std::string& token,
                       unsigned long octets[4],
                       int& outPort) {
    const size_t tn = token.size();
    if (tn == 0) return false;

    int octetIndex = 0;
    unsigned long currentValue = 0;
    int digitCount = 0;
    size_t p = 0;
    bool sawColon = false;

    // Parse up to four dot-separated octets.
    for (; p < tn; ++p) {
        char c = token[p];
        if (isDigitChar(c)) {
            ++digitCount;
            if (digitCount > 3) return false;              // >3 digits in an octet
            currentValue = currentValue * 10UL +
                           static_cast<unsigned long>(c - '0');
            if (currentValue > 255UL) return false;        // octet out of range
        } else if (c == '.') {
            if (digitCount == 0) return false;             // ".." or leading '.'
            if (octetIndex >= 3) return false;             // too many dots
            octets[octetIndex++] = currentValue;
            currentValue = 0;
            digitCount = 0;
        } else if (c == ':') {
            // Colon may only appear after the 4th octet's digits.
            if (octetIndex != 3 || digitCount == 0) return false;
            octets[octetIndex++] = currentValue;
            currentValue = 0;
            digitCount = 0;
            sawColon = true;
            ++p;                                           // step past ':'
            break;
        } else {
            // isTokenChar guaranteed one of digit/'.'/':' so this is unreachable.
            return false;
        }
    }

    // If we ended the loop without seeing a colon, close out the 4th octet.
    if (!sawColon) {
        if (octetIndex != 3 || digitCount == 0) return false;
        octets[octetIndex++] = currentValue;
    }

    if (octetIndex != 4) return false;

    // Handle the optional port.
    if (!sawColon) {
        outPort = -1;                                      // no port supplied
        return true;
    }

    // We just consumed the ':'. There must be 1..5 digits and nothing else.
    unsigned long portValue = 0;
    int portDigits = 0;
    for (; p < tn; ++p) {
        char c = token[p];
        if (!isDigitChar(c)) return false;                 // e.g. "1.2.3.4:80:9"
        ++portDigits;
        if (portDigits > 5) return false;
        portValue = portValue * 10UL +
                    static_cast<unsigned long>(c - '0');
        if (portValue > 65535UL) return false;
    }
    if (portDigits == 0) return false;                     // trailing ':' with no port

    outPort = static_cast<int>(portValue);
    return true;
}

bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort) {
    const size_t n = str.size();
    size_t i = 0;

    while (i < n) {
        // Skip characters that can't be part of a token.
        while (i < n && !isTokenChar(str[i])) ++i;
        if (i >= n) break;

        // Grab the maximal run of token characters.
        size_t start = i;
        while (i < n && isTokenChar(str[i])) ++i;
        std::string token = str.substr(start, i - start);

        unsigned long octets[4] = {0, 0, 0, 0};
        int port = -1;
        if (parseToken(token, octets, port)) {
            outAddress = (octets[0] << 24) |
                         (octets[1] << 16) |
                         (octets[2] <<  8) |
                          octets[3];
            outPort = port;
            return true;
        }
        // Otherwise, keep scanning for the next token.
    }

    return false;
}

int main() {
    std::string line;
    while (true) {
        std::cout << "Enter a string (or END to quit): "; // this line was manually written by Om Ghonasgi
        if (!std::getline(std::cin, line)) break;          // EOF -> exit
        if (line == "END") { 
            std::cout << "Program terminated." << std::endl; // this line was manually written by Om Ghonasgi
            break;                          // case-sensitive quit
        }
        unsigned long address = 0;
        int port = -1;
        if (extractIPv4(line, address, port)) {
            unsigned long a = (address >> 24) & 0xFFUL;
            unsigned long b = (address >> 16) & 0xFFUL;
            unsigned long c = (address >>  8) & 0xFFUL;
            unsigned long d =  address        & 0xFFUL;

            std::cout << "Extracted IPv4 address: "
                      << a << '.' << b << '.' << c << '.' << d
                      << " (decimal value: " << address
                      << ", port: ";
            if (port < 0) std::cout << "none";
            else          std::cout << port;
            std::cout << ")\n";
        } else {
            std::cout << "No valid IPv4 address found.\n";
        }
    }
    return 0;
}
