// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_io_thread.h"
#include "build/build_config.h"
#include "brave/test/base/brave_unit_test_suite.h"
#include "content/public/test/unittest_test_suite.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"

#if defined(OS_WIN)
#include "chrome/install_static/test/scoped_install_details.h"
#endif

int main(int argc, char **argv) {
  content::UnitTestTestSuite test_suite(new BraveUnitTestSuite(argc, argv));

  return base::LaunchUnitTests(
      argc, argv, base::Bind(&content::UnitTestTestSuite::Run,
                             base::Unretained(&test_suite)));
}
