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

const char* g_test_suite = "";
b8 g_test_failures_only = FALSE;

typedef enum _test_result {
  TEST_PASS,
  TEST_FAIL,
  TEST_XFAIL,
  TEST_XPASS,
  TEST_SKIP,
} test_result;

typedef struct _test_record {
  const char* suite;
  const char* name;
  test_result result;
  u64 issue;
  const char* note;
} test_record;

#define MAX_TEST_RECORDS 1024
static test_record records[MAX_TEST_RECORDS];
static u64 record_count = 0;

static void record_result(const char* name, test_result result, u64 issue, const char* note) {
  if(record_count < MAX_TEST_RECORDS) {
    records[record_count++] = (test_record){g_test_suite, name, result, issue, note};
  }
}

static void run_in_process(void (*fn)(void), b8 check_leaks) {
  u64 before = get_memory_allocated();
  u64 tags_before[MEMORY_TAG_END_TAG];
  for(int i = 0; i < MEMORY_TAG_END_TAG; i++) {
    tags_before[i] = get_memory_allocated_by_tag(i);
  }
  fn();
  u64 after = get_memory_allocated();
  if(check_leaks) {
    CHECK(after == before, "test leaked memory tracked by vmemory");
    // A block freed under a different tag than it was allocated with leaves
    // the total balanced but shifts bytes between tags, so check each tag too.
    b8 tags_balanced = TRUE;
    for(int i = 0; i < MEMORY_TAG_END_TAG; i++) {
      u64 tag_after = get_memory_allocated_by_tag(i);
      if(tag_after != tags_before[i]) {
        VERROR("    %s: %lld bytes outstanding", memory_tag_strings[i], (long long)(tag_after - tags_before[i]));
        tags_balanced = FALSE;
      }
    }
    CHECK(tags_balanced == TRUE, "every memory tag returns to where it started");
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
static void run_forked(const char* name, void (*fn)(void), b8 check_leaks, FILE* capture) {
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
    if(capture != NULL) {
      // The parent prints this output only if the test fails.
      dup2(fileno(capture), STDOUT_FILENO);
      dup2(fileno(capture), STDERR_FILENO);
    }
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
  char reason[256];
  if(WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM) {
    snprintf(reason, sizeof(reason), "timed out after %d seconds (infinite loop?)", TEST_CASE_TIMEOUT_SECONDS);
  } else if(WIFSIGNALED(status)) {
    snprintf(reason, sizeof(reason), "crashed with signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
  } else {
    snprintf(reason, sizeof(reason), "exited with status %d before finishing", WEXITSTATUS(status));
  }
  if(capture != NULL) {
    // Shown with the rest of the test's output, and only if it is shown.
    fseek(capture, 0, SEEK_END);
    fprintf(capture, "FAIL %s: %s\n", name, reason);
  } else {
    VERROR("FAIL %s: %s", name, reason);
  }
}

#endif

#if VPLATFORM_LINUX
// --failures captures each test's output in a temporary file. NULL otherwise.
static FILE* begin_capture(void) {
  if(!g_test_failures_only) {
    return NULL;
  }
  fflush(stdout);
  fflush(stderr);
  return tmpfile();
}

static void end_capture(FILE* capture, b8 show) {
  if(capture == NULL) {
    return;
  }
  if(show) {
    char buffer[4096];
    size_t n;
    rewind(capture);
    while((n = fread(buffer, 1, sizeof(buffer), capture)) > 0) {
      fwrite(buffer, 1, n, stdout);
    }
    fflush(stdout);
  }
  fclose(capture);
}
#endif

// Runs fn and returns how many checks it failed.
static u64 run_isolated(const char* name, void (*fn)(void), b8 check_leaks, b8 known_bug) {
  u64 failures_before = g_test_failures;
#if VPLATFORM_LINUX
  FILE* capture = begin_capture();
  run_forked(name, fn, check_leaks, capture);
  u64 failed = g_test_failures - failures_before;
  // A known bug's failure is expected, so its output is shown only when it
  // unexpectedly passes (there is none then) or for ordinary failures.
  end_capture(capture, failed > 0 && !known_bug);
#else
  run_in_process(fn, check_leaks);
  u64 failed = g_test_failures - failures_before;
#endif
  return failed;
}

static void announce(const char* name) {
  if(!g_test_failures_only) {
    VINFO("  %s", name);
  }
}

void run_test_case(const char* name, void (*fn)(void), b8 check_leaks) {
  announce(name);
  u64 failed = run_isolated(name, fn, check_leaks, FALSE);
  if(failed > 0) {
    if(g_test_failures_only) {
      VERROR("FAIL %s / %s (output above)", g_test_suite, name);
    }
    record_result(name, TEST_FAIL, 0, NULL);
  } else {
    record_result(name, TEST_PASS, 0, NULL);
  }
}

void run_known_bug_case(const char* name, void (*fn)(void), u64 issue, u32 flags) {
#if !VPLATFORM_LINUX
  (void)fn;
  (void)flags;
  announce(name);
  record_result(name, TEST_SKIP, issue, "known-bug tests only run on Linux");
  return;
#else
#ifndef VELORA_TEST_SANITIZERS
  if(flags & KNOWN_BUG_SANITIZER_ONLY) {
    announce(name);
    record_result(name, TEST_SKIP, issue, "only a sanitizer build can see this bug");
    return;
  }
#endif
  if(!g_test_failures_only) {
    VINFO("  %s (known bug #%llu, expected to fail)", name, (unsigned long long)issue);
  }
  u64 failed = run_isolated(name, fn, (flags & KNOWN_BUG_IGNORE_LEAKS) == 0, TRUE);
  if(failed > 0) {
    // Expected: take its failures back out of the suite's count.
    g_test_failures -= failed;
    record_result(name, TEST_XFAIL, issue, NULL);
  } else {
    g_test_failures++;
    VERROR("XPASS %s / %s: known bug #%llu no longer fails. Switch the test to RUN_TEST and close #%llu.",
      g_test_suite, name, (unsigned long long)issue, (unsigned long long)issue);
    record_result(name, TEST_XPASS, issue, NULL);
  }
#endif
}

static const char* result_names[] = {"PASS", "FAIL", "XFAIL", "XPASS", "SKIP"};

static void print_records(test_result result) {
  for(u64 i = 0; i < record_count; i++) {
    test_record* r = &records[i];
    if(r->result != result) {
      continue;
    }
    if(r->issue == 0) {
      if(result == TEST_FAIL) {
        VERROR("  %-5s %s / %s", result_names[result], r->suite, r->name);
      } else {
        VINFO("  %-5s %s / %s", result_names[result], r->suite, r->name);
      }
    } else if(result == TEST_XPASS) {
      VERROR("  %-5s %s / %s (#%llu is fixed: use RUN_TEST, close the issue)", result_names[result], r->suite, r->name, (unsigned long long)r->issue);
    } else if(r->note != NULL) {
      VINFO("  %-5s %s / %s (#%llu, %s)", result_names[result], r->suite, r->name, (unsigned long long)r->issue, r->note);
    } else {
      VINFO("  %-5s %s / %s (#%llu)", result_names[result], r->suite, r->name, (unsigned long long)r->issue);
    }
  }
}

void print_test_summary(void) {
  u64 counts[5] = {0};
  for(u64 i = 0; i < record_count; i++) {
    counts[records[i].result]++;
  }
  VINFO("Summary: %llu passed, %llu failed, %llu known bugs still failing, %llu known bugs fixed, %llu skipped",
    (unsigned long long)counts[TEST_PASS], (unsigned long long)counts[TEST_FAIL],
    (unsigned long long)counts[TEST_XFAIL], (unsigned long long)counts[TEST_XPASS],
    (unsigned long long)counts[TEST_SKIP]);
  print_records(TEST_FAIL);
  print_records(TEST_XPASS);
  print_records(TEST_XFAIL);
  print_records(TEST_SKIP);
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
