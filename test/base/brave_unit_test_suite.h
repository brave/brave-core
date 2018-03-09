// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_TEST_BASE_BRAVE_UNIT_TEST_SUITE_H_
#define BRAVE_TEST_BASE_BRAVE_UNIT_TEST_SUITE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/test/test_discardable_memory_allocator.h"
#include "chrome/test/base/chrome_unit_test_suite.h"

class BraveUnitTestSuite : public ChromeUnitTestSuite {
 public:
  BraveUnitTestSuite(int argc, char** argv);

 protected:
  // base::TestSuite overrides:
  void Initialize() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveUnitTestSuite);
};

#endif  // BRAVE_TEST_BASE_BRAVE_UNIT_TEST_SUITE_H_
