# 581-A1-Extracting-IPv4-Addresses-from-Noisy-Text

## AI Disclosure

### Tool and model used
- **Cursor** (agent mode) with **Claude Opus 4.7**.

### Dates AI was consulted
- **9/3/26** — initial C++ program, leading-zero fix for octets, extending the same rule to ports, running exploratory test cases in the terminal.
- **9/4/26** — moving the test cases into a dedicated C++ file, tightening the port rule to require *continuous* port digits (letter interruption and then comma interruption), and drafting/updating this disclosure.

### Prompts I gave, in order
1. **9/3/26** — Write a C++ program that reads a line of text and extracts a single valid IPv4 embedded anywhere in the text. Only read digits, periods, and colons are ever part of a valid token, any other character can be skipped - for example, if the character "a" appears in between a valid and invalid address, only read the valid one. exactly one valid token may be extracted per line. so if there are multiple valid addresses per line, only read the first one. When successful, print: Extracted IPv4 address: A.B.C.D (decimal value: N, port: P) where N is the 32-bit decimal value and P is the port number or the literal text none. Make sure there is a main loop that continuously prompts the user until they enter "END" (case senstive) and make sure it uses the function extractIPv4 with this exact function signiature:
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort);
Do not use atoi, atol, atoll, strtol, strtoul, strtod, stoi, stol, stoul, sscanf, inet_aton, inet_pton, inet_addr, or any regex library.
2. **9/3/26** — "From this test case: `092.168.1.1`. There should be no leading zeros in each octet unless the value is exactly zero — correct this." The assistant added a leading-zero check to `parseToken` in `ipv4_extract.cpp` (rejects `092`, `01`, `001`, etc., but still accepts a bare `0`).
3. **9/3/26** — "Apply this same principle to ports as well — only accept valid ports (i.e. no leading zeros, 0 – 65535, no other characters)." The assistant added the same leading-zero check to the port-parsing loop in `parseToken`.
4. **9/3/26 – 9/4/26** — Run and print out test cases for many different scenarios as well as edge cases: leading zeros, invalid characters between digits, ports out of range, incomplete addresses, periods or colons in invalid places. Move these test cases into a CPP file in this directory.
The assistant first piped a large batch of inputs through the compiled program in the terminal and produced a pass/fail table (on 9/3/26), then moved everything into a dedicated C++ test file (on 9/4/26):
   - Added an `#ifndef IPV4_EXTRACT_NO_MAIN … #endif` guard around `main()` in `ipv4_extract.cpp` so the test driver can reuse `extractIPv4` without duplicating logic.
   - Created `test_ipv4_extract.cpp`, which `#define`s `IPV4_EXTRACT_NO_MAIN`, `#include`s `ipv4_extract.cpp`, and runs the test cases across 7 categories (`valid`, `leading zero`, `port`, `octet range`, `structure`, `garbage`, `empty`), printing an aligned pass/fail table.
   - Built with `g++ -std=c++17 -Wall -Wextra` and ran the suite.
5. **9/4/26** — "In my ReadMe, draft an AI disclosure document with the following: What model I used (cursor with Claude Opus 4.7), all the prompts I have given in order, and acknowledge any bugs or limitations. Also include the dates that AI was consulted (9/3/26 and 9/4/26)." The assistant appended this disclosure section to the README.
6. **9/4/26** — "`10.0.0.1:8a0` — this test case should be rejected because there is a character in between two digits which would result in a correct port number. Port numbers should only be accepted if the digits are continuous; however, if there are garbage characters after the valid port number, it should be accepted. For example: `10.0.0.1:8a0` should be rejected but `10.0.0.255:8080end` should be accepted with port `8080`." The assistant added a continuous-port check in `extractIPv4`: after `parseToken` succeeds, it scans forward in the source string and rejects the match if a digit appears in the identifier-continuation run that follows a letter/underscore right after the token.
7. **9/4/26** — "`10.0.0.1:80,42` — this test case should also be rejected because although it is a comma and not a character, there should be no interruption between valid port number digits." The assistant tightened the continuous-port check so that *any* non-whitespace character between the port digits and further digits invalidates the match. The only clean boundary is now whitespace (space, tab, newline, carriage return) or end-of-string. Test suite grew to 54 cases; all pass.
8. **9/4/26** — "Update my ReadMe to include these new prompts and remove the old behavior of `10.0.0.1:8a0 → port 8` from the bugs and limitations."

### Bugs, limitations, and caveats surfaced during AI-assisted work
- **First round of tests had two false negatives caused by my expected values, not the code.** When the initial batch ran, two cases were flagged FAIL:
  - `.1.2.3.4` (leading dot) — I initially expected the parser to recover `1.2.3.4`.
  - `:10.0.0.1` (leading colon) — I initially expected the parser to recover `10.0.0.1`.

  On review, both are the *correct* behavior given the program's tokenization rule (a token is a maximal run of digits, `.`, and `:`). The whole string is one token that begins with `.`/`:`, so it is legitimately rejected. The expected values in `test_ipv4_extract.cpp` were updated to match.
- **Port-continuity rule evolved during development.** The initial extractor accepted `10.0.0.1:8a0` as `10.0.0.1:8` (because `a` is a non-token character that splits the token, leaving `10.0.0.1:8` internally valid). Prompt 6 tightened this so that a letter/underscore between port digits invalidates the match. Prompt 7 tightened it further so that *any* non-whitespace character between port digits and more digits invalidates the match (e.g. `10.0.0.1:80,42` is now rejected). The current rule is: after the port digits, the only clean boundary is whitespace or end-of-string; any digit appearing after a non-whitespace interruption invalidates the match.

### What was authored by hand vs. AI
- Two comment lines in `main()` of `ipv4_extract.cpp` are explicitly marked "this line was manually written by Om Ghonasgi."
- Produced by the AI assistant based on the prompts above:
  - The leading-zero checks for octets and ports in `parseToken`.
  - The continuous-port check in `extractIPv4` (with the `isWhitespaceChar` helper and the updated file-level comment describing the rule).
  - The `#ifndef IPV4_EXTRACT_NO_MAIN` guard around `main()`.
  - The entire contents of `test_ipv4_extract.cpp` (currently 54 cases, all passing).

## Build and run

```shell
# Build the main program
g++ -std=c++17 -Wall -Wextra -o ipv4_extract ipv4_extract.cpp

# Run it (interactive; type END to quit)
./ipv4_extract

# Build and run the test suite
g++ -std=c++17 -Wall -Wextra -o test_ipv4_extract test_ipv4_extract.cpp
./test_ipv4_extract
```
