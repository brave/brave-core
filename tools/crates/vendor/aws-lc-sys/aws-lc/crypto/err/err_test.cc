// Copyright (c) 2014, Google Inc.
// SPDX-License-Identifier: ISC

#include <stdio.h>
#include <string.h>

#include <thread>

#include <gtest/gtest.h>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/mem.h>

#include "./internal.h"

#if defined(OPENSSL_WINDOWS)
OPENSSL_MSVC_PRAGMA(warning(push, 3))
#include <windows.h>
OPENSSL_MSVC_PRAGMA(warning(pop))
#else
#include <errno.h>
#endif


TEST(ErrTest, Overflow) {
  for (unsigned i = 0; i < ERR_NUM_ERRORS*2; i++) {
    ERR_put_error(1, 0 /* unused */, i+1, "test", 1);
  }

  for (unsigned i = 0; i < ERR_NUM_ERRORS - 1; i++) {
    SCOPED_TRACE(i);
    uint32_t err = ERR_get_error();
    // Errors are returned in order they were pushed, with the least recent ones
    // removed, up to |ERR_NUM_ERRORS - 1| errors. So the errors returned are
    // |ERR_NUM_ERRORS + 2| through |ERR_NUM_ERRORS * 2|, inclusive.
    EXPECT_NE(0u, err);
    EXPECT_EQ(static_cast<int>(i + ERR_NUM_ERRORS + 2), ERR_GET_REASON(err));
  }

  EXPECT_EQ(0u, ERR_get_error());
}

TEST(ErrTest, PutError) {
  ASSERT_EQ(0u, ERR_get_error())
      << "ERR_get_error returned value before an error was added.";

  ERR_put_error(1, 0 /* unused */, 2, "test", 4);
  ERR_add_error_data(1, "testing");

  int peeked_line, line, peeked_flags, flags;
  const char *peeked_file, *file, *peeked_data, *data;
  uint32_t peeked_packed_error =
      ERR_peek_error_line_data(&peeked_file, &peeked_line, &peeked_data,
                               &peeked_flags);
  uint32_t packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);

  EXPECT_EQ(peeked_packed_error, packed_error);
  EXPECT_EQ(peeked_file, file);
  EXPECT_EQ(peeked_data, data);
  EXPECT_EQ(peeked_flags, flags);

  EXPECT_STREQ("test", file);
  EXPECT_EQ(4, line);
  EXPECT_EQ(flags, ERR_FLAG_STRING | ERR_FLAG_MALLOCED);
  EXPECT_EQ(1, ERR_GET_LIB(packed_error));
  EXPECT_EQ(2, ERR_GET_REASON(packed_error));
  EXPECT_STREQ("testing", data);

  ERR_put_error(1, 0 /* unused */, 2, "test", 4);
  ERR_set_error_data(const_cast<char *>("testing"), ERR_FLAG_STRING);
  packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
  EXPECT_NE(0u, packed_error);
  EXPECT_STREQ("testing", data);

  ERR_put_error(1, 0 /* unused */, 2, "test", 4);
  bssl::UniquePtr<char> str(OPENSSL_strdup("testing"));
  ERR_set_error_data(str.release(), ERR_FLAG_STRING | ERR_FLAG_MALLOCED);
  packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
  EXPECT_NE(0u, packed_error);
  EXPECT_STREQ("testing", data);
}

TEST(ErrTest, ClearError) {
  ASSERT_EQ(0u, ERR_get_error())
      << "ERR_get_error returned value before an error was added.";

  ERR_put_error(1, 0 /* unused */, 2, "test", 4);
  ERR_clear_error();

  // The error queue should be cleared.
  EXPECT_EQ(0u, ERR_get_error());
}

TEST(ErrTest, Print) {
  ERR_put_error(1, 0 /* unused */, 2, "test", 4);
  ERR_add_error_data(1, "testing");
  uint32_t packed_error = ERR_get_error();

  char buf[256];
  for (size_t i = 0; i <= sizeof(buf); i++) {
    ERR_error_string_n(packed_error, buf, i);
  }
}

TEST(ErrTest, Release) {
  ERR_put_error(1, 0 /* unused */, 2, "test", 4);
  ERR_remove_thread_state(NULL);

  // The error queue should be cleared.
  EXPECT_EQ(0u, ERR_get_error());
}

static bool HasSuffix(const char *str, const char *suffix) {
  size_t suffix_len = strlen(suffix);
  size_t str_len = strlen(str);
  if (str_len < suffix_len) {
    return false;
  }
  return strcmp(str + str_len - suffix_len, suffix) == 0;
}

TEST(ErrTest, PutMacro) {
  int expected_line = __LINE__ + 1;
  OPENSSL_PUT_ERROR(USER, ERR_R_INTERNAL_ERROR);

  int line;
  const char *file;
  uint32_t error = ERR_get_error_line(&file, &line);

  EXPECT_PRED2(HasSuffix, file, "err_test.cc");
  EXPECT_EQ(expected_line, line);
  EXPECT_EQ(ERR_LIB_USER, ERR_GET_LIB(error));
  EXPECT_EQ(ERR_R_INTERNAL_ERROR, ERR_GET_REASON(error));
}

TEST(ErrTest, SaveAndRestore) {
  // Restoring no state clears the error queue, including error data.
  ERR_put_error(1, 0 /* unused */, 1, "test1.c", 1);
  ERR_put_error(2, 0 /* unused */, 2, "test2.c", 2);
  ERR_add_error_data(1, "data1");
  ERR_restore_state(nullptr);
  EXPECT_EQ(0u, ERR_get_error());

  // Add some entries to the error queue and save it.
  ERR_put_error(1, 0 /* unused */, 1, "test1.c", 1);
  ERR_add_error_data(1, "data1");
  ERR_put_error(2, 0 /* unused */, 2, "test2.c", 2);
  ERR_put_error(3, 0 /* unused */, 3, "test3.c", 3);
  ERR_add_error_data(1, "data3");
  bssl::UniquePtr<ERR_SAVE_STATE> saved(ERR_save_state());
  ASSERT_TRUE(saved);

  // The existing error queue entries still exist.
  int line, flags;
  const char *file, *data;
  uint32_t packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
  EXPECT_EQ(ERR_GET_LIB(packed_error), 1);
  EXPECT_EQ(ERR_GET_REASON(packed_error), 1);
  EXPECT_STREQ("test1.c", file);
  EXPECT_EQ(line, 1);
  EXPECT_STREQ(data, "data1");
  EXPECT_EQ(flags, ERR_FLAG_STRING | ERR_FLAG_MALLOCED);

  // The state may be restored, both over an empty and non-empty state.
  for (unsigned i = 0; i < 2; i++) {
    SCOPED_TRACE(i);
    ERR_restore_state(saved.get());

    packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
    EXPECT_EQ(ERR_GET_LIB(packed_error), 1);
    EXPECT_EQ(ERR_GET_REASON(packed_error), 1);
    EXPECT_STREQ("test1.c", file);
    EXPECT_EQ(line, 1);
    EXPECT_STREQ(data, "data1");
    EXPECT_EQ(flags, ERR_FLAG_STRING | ERR_FLAG_MALLOCED);

    packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
    EXPECT_EQ(ERR_GET_LIB(packed_error), 2);
    EXPECT_EQ(ERR_GET_REASON(packed_error), 2);
    EXPECT_STREQ("test2.c", file);
    EXPECT_EQ(line, 2);
    EXPECT_STREQ(data, "");  // No error data is reported as the empty string.
    EXPECT_EQ(flags, 0);

    packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
    EXPECT_EQ(ERR_GET_LIB(packed_error), 3);
    EXPECT_EQ(ERR_GET_REASON(packed_error), 3);
    EXPECT_STREQ("test3.c", file);
    EXPECT_EQ(line, 3);
    EXPECT_STREQ(data, "data3");
    EXPECT_EQ(flags, ERR_FLAG_STRING | ERR_FLAG_MALLOCED);

    // The error queue is now empty for the next iteration.
    EXPECT_EQ(0u, ERR_get_error());
  }

  // Test a case where the error queue wraps around. The first set of errors
  // will all be discarded, but result in wrapping the list around.
  ERR_clear_error();
  for (unsigned i = 0; i < ERR_NUM_ERRORS / 2; i++) {
    ERR_put_error(0, 0 /* unused */, 0, "invalid", 0);
  }
  for (unsigned i = 1; i < ERR_NUM_ERRORS; i++) {
    ERR_put_error(i, 0 /* unused */, i, "test", i);
  }
  saved.reset(ERR_save_state());

  // The state may be restored, both over an empty and non-empty state. Pop one
  // error off so the first iteration is tested to not be a no-op.
  ERR_get_error();
  for (int i = 0; i < 2; i++) {
    SCOPED_TRACE(i);
    ERR_restore_state(saved.get());
    for (int j = 1; j < ERR_NUM_ERRORS; j++) {
      SCOPED_TRACE(j);
      packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
      EXPECT_EQ(ERR_GET_LIB(packed_error), j);
      EXPECT_EQ(ERR_GET_REASON(packed_error), j);
      EXPECT_STREQ("test", file);
      EXPECT_EQ(line, j);
    }
    // The error queue is now empty for the next iteration.
    EXPECT_EQ(0u, ERR_get_error());
  }
}

// A mark is not part of a saved state. A snapshot may be restored many times, so
// a mark carried in one would be re-armed on every restore and a later
// |ERR_pop_to_mark| would succeed against a mark nobody set.
TEST(ErrTest, SaveAndRestoreDropsMark) {
  ERR_clear_error();
  ERR_put_error(1, 0 /* unused */, 1, "test1.c", 1);
  ASSERT_TRUE(ERR_set_mark());

  bssl::UniquePtr<ERR_SAVE_STATE> saved(ERR_save_state());
  ASSERT_TRUE(saved);
  ERR_restore_state(saved.get());

  EXPECT_FALSE(ERR_pop_to_mark());
  EXPECT_EQ(0u, ERR_get_error());
}

TEST(ErrTest, SuppressErrors) {
  ERR_clear_error();

  ASSERT_TRUE(ERR_suppress_errors_begin());
  for (unsigned i = 1; i <= 4; i++) {
    ERR_put_error(i, 0 /* unused */, i, "test", i);
  }
  EXPECT_EQ(0u, ERR_peek_error());
  ERR_suppress_errors_end();

  // The scope is over, so errors are recorded again.
  ERR_put_error(5, 0 /* unused */, 5, "test", 5);
  EXPECT_EQ(ERR_GET_LIB(ERR_get_error()), 5);
  EXPECT_EQ(0u, ERR_get_error());
}

// A saturated queue is what suppression is for: each error raised inside the
// scope would otherwise evict one of the caller's, and trimming the queue
// afterward does not bring an evicted entry back.
TEST(ErrTest, SuppressErrorsPreservesFullQueue) {
  ERR_clear_error();
  for (unsigned i = 1; i < ERR_NUM_ERRORS; i++) {
    ERR_put_error(1, 0 /* unused */, i, "test", 1);
  }

  ASSERT_TRUE(ERR_suppress_errors_begin());
  for (unsigned i = 0; i < ERR_NUM_ERRORS * 2; i++) {
    ERR_put_error(2, 0 /* unused */, 2, "test", 2);
  }
  ERR_suppress_errors_end();

  for (unsigned i = 1; i < ERR_NUM_ERRORS; i++) {
    uint32_t packed_error = ERR_get_error();
    EXPECT_EQ(ERR_GET_LIB(packed_error), 1);
    EXPECT_EQ(ERR_GET_REASON(packed_error), static_cast<int>(i));
  }
  EXPECT_EQ(0u, ERR_get_error());
}

// Data attaches to the most recent error, so suppressing the |ERR_put_error| but
// not the |ERR_add_error_data| that follows it would overwrite the data on
// whatever the caller had queued last.
TEST(ErrTest, SuppressErrorsLeavesCallerDataAlone) {
  ERR_clear_error();
  ERR_put_error(1, 0 /* unused */, 1, "test1.c", 1);
  ERR_add_error_data(1, "data1");
  ASSERT_TRUE(ERR_set_mark());

  ASSERT_TRUE(ERR_suppress_errors_begin());
  ERR_put_error(2, 0 /* unused */, 2, "test2.c", 2);
  ERR_add_error_data(1, "data2");
  ERR_add_error_dataf("data%d", 3);
  ERR_suppress_errors_end();

  EXPECT_TRUE(ERR_pop_to_mark());
  int line, flags;
  const char *file, *data;
  uint32_t packed_error = ERR_get_error_line_data(&file, &line, &data, &flags);
  EXPECT_EQ(ERR_GET_LIB(packed_error), 1);
  EXPECT_EQ(ERR_GET_REASON(packed_error), 1);
  EXPECT_STREQ("test1.c", file);
  EXPECT_EQ(line, 1);
  EXPECT_STREQ(data, "data1");
  EXPECT_EQ(0u, ERR_get_error());
}

// The caller may be holding a string it read without popping the error, which
// data reaching that error would free under it. Scoped work that attaches data
// without raising an error of its own writes to exactly that error.
TEST(ErrTest, SuppressErrorsKeepsReadDataAlive) {
  ERR_clear_error();
  ERR_put_error(1, 0 /* unused */, 1, "test1.c", 1);
  ERR_add_error_data(1, "data1");

  int line, flags;
  const char *file, *data;
  uint32_t packed_error =
      ERR_peek_error_line_data(&file, &line, &data, &flags);
  ASSERT_EQ(ERR_GET_LIB(packed_error), 1);
  ASSERT_STREQ(data, "data1");

  ASSERT_TRUE(ERR_suppress_errors_begin());
  ERR_add_error_data(1, "data2");
  ERR_add_error_dataf("data%d", 3);
  ERR_suppress_errors_end();

  EXPECT_STREQ(data, "data1");
  ERR_clear_error();
}

// Ending a scope that never opened would decrement a depth nothing incremented,
// so the C++ holder ends only what it began and says which it did.
TEST(ErrTest, ScopedErrorSuppression) {
  ERR_clear_error();

  {
    bssl::ScopedErrorSuppression suppress;
    ASSERT_TRUE(static_cast<bool>(suppress));
    ERR_put_error(1, 0 /* unused */, 1, "test", 1);
  }

  EXPECT_EQ(0u, ERR_peek_error());
  ERR_put_error(2, 0 /* unused */, 2, "test", 2);
  EXPECT_EQ(ERR_GET_LIB(ERR_get_error()), 2);
  EXPECT_EQ(0u, ERR_get_error());
}

TEST(ErrTest, SuppressErrorsNests) {
  ERR_clear_error();

  ASSERT_TRUE(ERR_suppress_errors_begin());
  ASSERT_TRUE(ERR_suppress_errors_begin());
  ERR_suppress_errors_end();

  // The outer scope is still open.
  ERR_put_error(1, 0 /* unused */, 1, "test", 1);
  EXPECT_EQ(0u, ERR_peek_error());

  ERR_suppress_errors_end();
  ERR_put_error(2, 0 /* unused */, 2, "test", 2);
  EXPECT_EQ(ERR_GET_LIB(ERR_get_error()), 2);
  EXPECT_EQ(0u, ERR_get_error());
}

// A scope can be the first thing a thread does with the error queue, which is
// where |ERR_suppress_errors_begin| has to create the thread's error state and
// so is the one case where it can fail. Every other test here clears the queue
// first and therefore never exercises that path.
#if defined(OPENSSL_THREADS)
TEST(ErrTest, SuppressErrorsOnFreshThread) {
  std::thread([] {
    ASSERT_TRUE(ERR_suppress_errors_begin());
    ERR_put_error(1, 0 /* unused */, 1, "test", 1);
    ERR_suppress_errors_end();
    EXPECT_EQ(0u, ERR_get_error());
  }).join();
}
#endif  // OPENSSL_THREADS

// Querying the error queue should not affect the OS error.
#if defined(OPENSSL_WINDOWS)
TEST(ErrTest, PreservesLastError) {
  SetLastError(ERROR_INVALID_FUNCTION);
  ERR_get_error();
  EXPECT_EQ(static_cast<DWORD>(ERROR_INVALID_FUNCTION), GetLastError());
}
#else
TEST(ErrTest, PreservesErrno) {
  errno = EINVAL;
  ERR_get_error();
  EXPECT_EQ(EINVAL, errno);
}
#endif

TEST(ErrTest, String) {
  char buf[128];
  const uint32_t err = ERR_PACK(ERR_LIB_CRYPTO, ERR_R_INTERNAL_ERROR);

  EXPECT_STREQ(
      "error:0e000044:common libcrypto routines:OPENSSL_internal:internal "
      "error",
      ERR_error_string_n(err, buf, sizeof(buf)));

  // The buffer is exactly the right size.
  EXPECT_STREQ(
      "error:0e000044:common libcrypto routines:OPENSSL_internal:internal "
      "error",
      ERR_error_string_n(err, buf, 73));

  // If the buffer is too short, the string is truncated.
  EXPECT_STREQ(
      "error:0e000044:common libcrypto routines:OPENSSL_internal:internal "
      "erro",
      ERR_error_string_n(err, buf, 72));
  EXPECT_STREQ("error:0e000044:common libcrypto routines:OPENSSL_internal:",
               ERR_error_string_n(err, buf, 59));

  // Truncated log lines always have the right number of colons.
  EXPECT_STREQ("error:0e000044:common libcrypto routines:OPENSSL_interna:",
               ERR_error_string_n(err, buf, 58));
  EXPECT_STREQ("error:0e000044:common libcrypto routines:OPENSSL_intern:",
               ERR_error_string_n(err, buf, 57));
  EXPECT_STREQ("error:0e000044:common libcryp::",
               ERR_error_string_n(err, buf, 32));
  EXPECT_STREQ("error:0e0000:::",
               ERR_error_string_n(err, buf, 16));
  EXPECT_STREQ("err::::",
               ERR_error_string_n(err, buf, 8));
  EXPECT_STREQ("::::",
               ERR_error_string_n(err, buf, 5));

  // If the buffer is too short for even four colons, |ERR_error_string_n| does
  // not bother trying to preserve the format.
  EXPECT_STREQ("err", ERR_error_string_n(err, buf, 4));
  EXPECT_STREQ("er", ERR_error_string_n(err, buf, 3));
  EXPECT_STREQ("e", ERR_error_string_n(err, buf, 2));
  EXPECT_STREQ("", ERR_error_string_n(err, buf, 1));

  // A buffer length of zero should not touch the buffer.
  ERR_error_string_n(err, nullptr, 0);
}

// Error-printing functions should return something with unknown errors.
TEST(ErrTest, UnknownError) {
  uint32_t err = ERR_PACK(0xff, 0xfff);
  EXPECT_TRUE(ERR_lib_error_string(err));
  EXPECT_TRUE(ERR_reason_error_string(err));
  char buf[128];
  ERR_error_string_n(err, buf, sizeof(buf));
  EXPECT_NE(0u, strlen(buf));
}
