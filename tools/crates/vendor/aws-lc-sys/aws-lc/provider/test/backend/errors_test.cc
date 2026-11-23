// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// The AWS-LC side of the error bridge. Only this binary can force a record onto
// AWS-LC's queue, so the translation is covered here rather than through the
// provider interface. test/frontend/provider_test.cc covers the front side.

#include <gtest/gtest.h>

#include <openssl/err.h>

#include <string>

#include "internal/backend.h"

namespace {

// EC reason 100 and BN reason 100 are different errors, which is what makes the
// library tag observable.
constexpr int kEcBufferTooSmall = 100;
constexpr int kBnArg2LtArg3 = 100;

class BackendErrorTest : public ::testing::Test {
 protected:
  void SetUp() override { ERR_clear_error(); }
  void TearDown() override { ERR_clear_error(); }

    static void Put(int library, int reason, const char *file, unsigned line) {
    ERR_put_error(library, 0, reason, file, line);
  }

  static AWSLC_PROV_ERROR Shift() {
    AWSLC_PROV_ERROR record;
    EXPECT_TRUE(awslc_prov_error_shift(&record));
    return record;
  }
};

TEST_F(BackendErrorTest, TagsReasonsWithTheirAwslcLibrary) {
  Put(ERR_LIB_EC, kEcBufferTooSmall, "ec.c", 1);
  Put(ERR_LIB_BN, kBnArg2LtArg3, "bn.c", 2);

  const AWSLC_PROV_ERROR ec = Shift();
  const AWSLC_PROV_ERROR bn = Shift();

  EXPECT_EQ(AWSLC_PROV_ERROR_REASON(ERR_LIB_EC, kEcBufferTooSmall), ec.reason);
  EXPECT_EQ(AWSLC_PROV_ERROR_REASON(ERR_LIB_BN, kBnArg2LtArg3), bn.reason);
  EXPECT_NE(ec.reason, bn.reason);
}

// The detail is the only place the AWS-LC library or reason name reaches the
// application, since the front side registers no reason string for an AWS-LC code.
TEST_F(BackendErrorTest, CarriesAwslcLibraryAndReasonAsDetail) {
  Put(ERR_LIB_EC, kEcBufferTooSmall, "ec.c", 1);

  const AWSLC_PROV_ERROR record = Shift();
  const std::string detail(record.detail);

  EXPECT_NE(std::string::npos, detail.find("AWS-LC"));
  EXPECT_NE(std::string::npos,
            detail.find(ERR_lib_error_string(
                ERR_PACK(ERR_LIB_EC, kEcBufferTooSmall))))
      << detail;
  EXPECT_NE(std::string::npos,
            detail.find(ERR_reason_error_string(
                ERR_PACK(ERR_LIB_EC, kEcBufferTooSmall))))
      << detail;
}

// AWS-LC's data pointer belongs to the queue and dies on the next call that
// touches it, so the shift has to copy it.
TEST_F(BackendErrorTest, CopiesTheAwslcRecordDetail) {
  Put(ERR_LIB_EC, kEcBufferTooSmall, "ec.c", 1);
  ERR_add_error_data(1, "curve P-256");
  Put(ERR_LIB_BN, kBnArg2LtArg3, "bn.c", 2);

  const AWSLC_PROV_ERROR first = Shift();
  // This shift invalidates |first|'s original data pointer.
  const AWSLC_PROV_ERROR second = Shift();

  EXPECT_NE(std::string::npos, std::string(first.detail).find("curve P-256"))
      << first.detail;
  EXPECT_EQ(std::string::npos, std::string(second.detail).find("curve P-256"))
      << second.detail;
}

TEST_F(BackendErrorTest, CarriesTheAwslcOrigin) {
  Put(ERR_LIB_EC, kEcBufferTooSmall, "some/file.c", 4321);

  const AWSLC_PROV_ERROR record = Shift();

  ASSERT_NE(nullptr, record.file);
  EXPECT_STREQ("some/file.c", record.file);
  EXPECT_EQ(4321, record.line);
}

TEST_F(BackendErrorTest, ShiftsOldestFirst) {
  Put(ERR_LIB_EC, kEcBufferTooSmall, "first.c", 1);
  Put(ERR_LIB_BN, kBnArg2LtArg3, "second.c", 2);

  EXPECT_STREQ("first.c", Shift().file);
  EXPECT_STREQ("second.c", Shift().file);

  AWSLC_PROV_ERROR record;
  EXPECT_FALSE(awslc_prov_error_shift(&record));
}

// A zero reason names nothing and an over-wide library cannot round-trip. Both
// must stay visible as a failure rather than resolve to another library's reason.
TEST_F(BackendErrorTest, SubstitutesItsOwnReasonWhenAwslcCannotBeTagged) {
  Put(ERR_LIB_EC, 0, "ec.c", 1);
  Put(AWSLC_PROV_ERROR_MAX_LIB + 1, kEcBufferTooSmall, "wide.c", 2);

  EXPECT_EQ((uint32_t)AWSLC_PROV_R_BACKEND_ERROR, Shift().reason);
  EXPECT_EQ((uint32_t)AWSLC_PROV_R_BACKEND_ERROR, Shift().reason);
}

// AWS-LC resolves a reason below 100 without the library field, so the bridge
// leaves those untagged and ERR_GET_REASON stays equal to what AWS-LC raised.
TEST_F(BackendErrorTest, LeavesCrossLibraryReasonsUntagged) {
  const int cross_library_reason = ERR_LIB_EC;

  Put(ERR_LIB_EVP, cross_library_reason, "evp.c", 1);

  EXPECT_EQ((uint32_t)cross_library_reason, Shift().reason);
}

TEST_F(BackendErrorTest, ClearDropsQueuedRecords) {
  Put(ERR_LIB_EC, kEcBufferTooSmall, "ec.c", 1);
  awslc_prov_error_clear();

  AWSLC_PROV_ERROR record;
  EXPECT_FALSE(awslc_prov_error_shift(&record));
}

}  // namespace
