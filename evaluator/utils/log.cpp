#include "log.h"

#include <iomanip>
#include <sstream>

#if defined(__unix__)
#include <sys/resource.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#endif

namespace utils {

timer::timer() { start(); }
void timer::start() { _start = clock::now(); }
double timer::elapsed() const { return std::chrono::duration<double>(clock::now() - _start).count(); }

timer tstamp;

NoStreamBuf nostreambuf;
std::ostream nocout(&nostreambuf);

std::ostream& operator<<(std::ostream& os, const timer& t) {
    std::ostringstream oss;  // seperate the impact of format
    oss << "[" << std::setprecision(3) << std::setw(8) << std::fixed << t.elapsed() << "] ";
    os << oss.str();
    return os;
}

double mem_use::get_current() {
#if defined(__unix__)
    long rss = 0L;
    FILE* fp = NULL;
    if ((fp = fopen("/proc/self/statm", "r")) == NULL) {
        return 0.0; /* Can't open? */
    }
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
        fclose(fp);
        return 0.0; /* Can't read? */
    }
    fclose(fp);
    return rss * sysconf(_SC_PAGESIZE) / 1048576.0;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return info.WorkingSetSize / 1048576.0;
#else
    return 0.0;  // unknown
#endif
}

double mem_use::get_peak() {
#if defined(__unix__)
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    return rusage.ru_maxrss / 1024.0;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info;
    GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
    return info.PeakWorkingSetSize / 1048576.0;
#else
    return 0.0;  // unknown
#endif
}

std::ostream& log(std::ostream& os) {
    os << tstamp;
    return os;
}

std::ostream& log(int log_level, std::ostream& os_ori) {
    std::ostream& os_new = (log_level >= GLOBAL_LOG_LEVEL) ? os_ori : nocout;
    std::string prefix = "";
    if (log_level > LOG_INFO) {
        prefix = log_level_ANSI_color(log_level);
    }
    os_new << tstamp << prefix;
    return os_new;
}

std::string log_level_ANSI_color(int& log_level) {
    std::string color_string;
    switch (log_level) {
        case LOG_NOTICE:
            color_string = "\033[1;34mNotice\033[0m: ";
            break;
        case LOG_WARN:
            color_string = "\033[1;93mWarning\033[0m: ";
            break;
        case LOG_ERROR:
            color_string = "\033[1;31mError\033[0m: ";
            break;
        case LOG_FATAL:
            color_string = "\033[1;41;97m F A T A L \033[0m: ";
            break;
        case LOG_OK:
            color_string = "\033[1;32mOK\033[0m: ";
            break;
        default:
            break;
    }
    return color_string;
}

void assert_msg(bool condition, const char* format, ...) {
    if (!condition) {
        std::cerr << "Assertion failure: ";
        va_list ap;
        va_start(ap, format);
        vfprintf(stdout, format, ap);
        std::cerr << std::endl;
        std::abort();
    }
    return;
}

}  // namespace utils