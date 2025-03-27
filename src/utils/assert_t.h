#pragma once

// work with static build
#define assert_t(ans) { assert_t_((ans), __FILE__, __LINE__); }
void assert_t_(bool ans, const char* file, int line, bool abort = true);