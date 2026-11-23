#include "test_framework.h"

#include <stdio.h>
#include <string.h>

#if VPLATFORM_LINUX
#include <signal.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// Seconds a single test case may run before it is killed and counted as a failure.
#define TEST_CASE_TIMEOUT_SECONDS 10

static void run_in_process(void (*fn)(void), b8 check_leaks) {
  u64 before = get_memory_allocated();
  fn();
  u64 after = get_memory_allocated();
  if(check_leaks) {
    CHECK(after == before, "test leaked memory tracked by vmemory");
  }
}

#if VPLATFORM_LINUX

/*
 * Runs the test in a forked child so a crash, sanitizer abort or hang only
 * fails this test. The child sends its check/failure counts back through a
 * pipe and leaves with _exit, which skips LeakSanitizer's exit-time scan: a
 * leak inside one test is reported by the vmemory check above instead, and
 * the parent still gets a full LeakSanitizer scan when it exits.
 */
static void run_forked(const char* name, void (*fn)(void), b8 check_leaks) {
  int fds[2];
  if(pipe(fds) != 0) {
    VWARN("    pipe() failed, running %s in-process", name);
    run_in_process(fn, check_leaks);
    return;
  }
  fflush(stdout);
  fflush(stderr);
  pid_t pid = fork();
  if(pid < 0) {
    close(fds[0]);
    close(fds[1]);
    VWARN("    fork() failed, running %s in-process", name);
    run_in_process(fn, check_leaks);
    return;
  }
  if(pid == 0) {
    close(fds[0]);
    // Crashing on a known bug is expected here; don't spend time writing core
    // dumps. A limit of 1 (not 0) also stops pipe handlers like systemd-coredump.
    struct rlimit no_core = {1, 1};
    setrlimit(RLIMIT_CORE, &no_core);
    alarm(TEST_CASE_TIMEOUT_SECONDS);
    g_test_failures = 0;
    g_test_checks = 0;
    run_in_process(fn, check_leaks);
    u64 counts[2] = {g_test_checks, g_test_failures};
    ssize_t written = write(fds[1], counts, sizeof(counts));
    (void)written;
    close(fds[1]);
    fflush(stdout);
    fflush(stderr);
    _exit(0);
  }

  close(fds[1]);
  u64 counts[2] = {0, 0};
  ssize_t got = 0;
  while(got < (ssize_t)sizeof(counts)) {
    ssize_t n = read(fds[0], ((u8*)counts) + got, sizeof(counts) - got);
    if(n <= 0) {
      break;
    }
    got += n;
  }
  close(fds[0]);
  int status = 0;
  waitpid(pid, &status, 0);

  b8 exited_cleanly = WIFEXITED(status) && WEXITSTATUS(status) == 0 && got == (ssize_t)sizeof(counts);
  if(exited_cleanly) {
    g_test_checks += counts[0];
    g_test_failures += counts[1];
    return;
  }
  g_test_checks++;
  g_test_failures++;
  if(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
    VERROR("FAIL %s: timed out after %d seconds (infinite loop?)", name, TEST_CASE_TIMEOUT_SECONDS);
  } else if(WIFSIGNALED(status)) {
    VERROR("FAIL %s: crashed with signal %d (%s)", name, WTERMSIG(status), strsignal(WTERMSIG(status)));
  } else {
    VERROR("FAIL %s: exited with status %d before finishing", name, WEXITSTATUS(status));
  }
}

#endif

void run_test_case(const char* name, void (*fn)(void), b8 check_leaks) {
  VINFO("  %s", name);
#if VPLATFORM_LINUX
  run_forked(name, fn, check_leaks);
#else
  run_in_process(fn, check_leaks);
#endif
}

b8 write_test_file(const char* path, const void* data, u64 size) {
  FILE* file = fopen(path, "wb");
  if(file == NULL) {
    VERROR("    could not create test file %s", path);
    return FALSE;
  }
  u64 written = size == 0 ? 0 : fwrite(data, 1, size, file);
  fclose(file);
  return written == size;
}

b8 write_test_text_file(const char* path, const char* text) {
  return write_test_file(path, text, strlen(text));
}

void remove_test_file(const char* path) {
  remove(path);
}
