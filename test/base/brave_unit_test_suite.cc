// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/test/base/brave_unit_test_suite.h"

#include "base/logging.h"
#include "brave/common/brave_paths.h"
#include "chrome/test/base/chrome_unit_test_suite.h"

BraveUnitTestSuite::BraveUnitTestSuite(int argc, char** argv)
    : ChromeUnitTestSuite(argc, argv) {}

void BraveUnitTestSuite::Initialize() {
  ChromeUnitTestSuite::Initialize();

  brave::RegisterPathProvider();
}
