// Runtime support for generated native code. This is not a C-source backend.
#ifdef _MSC_VER
#pragma runtime_checks("", off)
#endif
#ifdef _WIN32
extern "C" {
__declspec(dllimport) void* __stdcall GetStdHandle(unsigned long);
__declspec(dllimport) int __stdcall WriteFile(void*, const void*, unsigned long, unsigned long*, void*);
__declspec(dllimport) void __stdcall ExitProcess(unsigned int);
}
#else
#include <unistd.h>
#endif

static void reportFailure(int line, const char* reason) {
    char message[160]; unsigned size = 0;
    const char* prefix = "Error: line ";
    while (*prefix) message[size++] = *prefix++;
    unsigned value = line > 0 ? static_cast<unsigned>(line) : 0U;
    char digits[16]; unsigned count = 0;
    do { digits[count++] = static_cast<char>('0' + value % 10U); value /= 10U; } while (value);
    while (count) message[size++] = digits[--count];
    message[size++] = ':'; message[size++] = ' ';
    while (*reason) message[size++] = *reason++;
    message[size++] = '\n';
#ifdef _WIN32
    unsigned long written = 0;
    WriteFile(GetStdHandle(static_cast<unsigned long>(-12L)), message, size, &written, 0);
    ExitProcess(1);
#else
    unsigned sent = 0;
    while (sent < size) {
        const ssize_t written = write(2, message + sent, size - sent);
        if (written <= 0) break;
        sent += static_cast<unsigned>(written);
    }
    _exit(1);
#endif
}

extern "C" void __shalimarc_runtime_error_v1(int line, int code) {
    reportFailure(line, code == 1 ? "integer overflow" : code == 2 ? "division by zero" : "invalid conversion");
}

extern "C" void __shalimarc_array_bounds_v1(int line, int extent) {
    char reason[80]; unsigned size = 0;
    const char* prefix = "array index outside range [0, ";
    while (*prefix) reason[size++] = *prefix++;
    unsigned value = extent > 0 ? static_cast<unsigned>(extent-1) : 0;
    char digits[16]; unsigned count = 0;
    do { digits[count++] = static_cast<char>('0'+value%10); value /= 10; } while (value);
    while (count) reason[size++] = digits[--count];
    reason[size++] = ']'; reason[size] = 0;
    reportFailure(line, reason);
}
