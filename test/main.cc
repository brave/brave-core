/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "gtest/gtest.h"

int main(int argc, char** argv) {
  // The following line throws an exception on failure however can cause issues
  // on some compilers
  // ::testing::GTEST_FLAG(throw_on_failure) = true;

  // The following line must be executed to initialize Google Mock (and Google
  // Test) before running the tests.
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
