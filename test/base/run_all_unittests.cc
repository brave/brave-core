/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_io_thread.h"
#include "brave/test/base/brave_unit_test_suite.h"
#include "build/build_config.h"
#include "content/public/test/unittest_test_suite.h"
#include "mojo/core/embedder/scoped_ipc_support.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/install_static/test/scoped_install_details.h"
#endif

int main(int argc, char **argv) {
  content::UnitTestTestSuite test_suite(
      new BraveUnitTestSuite(argc, argv),
      base::BindRepeating(
          content::UnitTestTestSuite::CreateTestContentClients));

  base::TestIOThread test_io_thread(base::TestIOThread::kAutoStart);
  mojo::core::ScopedIPCSupport ipc_support(
      test_io_thread.task_runner(),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST);

#if BUILDFLAG(IS_WIN)
  install_static::ScopedInstallDetails scoped_install_details;
#endif

  return base::LaunchUnitTests(argc, argv,
                               base::BindOnce(&content::UnitTestTestSuite::Run,
                                              base::Unretained(&test_suite)));
}
