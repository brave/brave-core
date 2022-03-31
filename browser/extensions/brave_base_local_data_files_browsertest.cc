/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"

using brave_component_updater::LocalDataFilesObserver;
using content::BrowserThread;
using extensions::ExtensionBrowserTest;

void BaseLocalDataFilesBrowserTest::SetUp() {
  brave::RegisterPathProvider();
  MaybeInitEmbeddedTestServer();
  ExtensionBrowserTest::SetUp();
}

void BaseLocalDataFilesBrowserTest::SetUpOnMainThread() {
  MaybeSetUpEmbeddedTestServerOnMainThread();
  ExtensionBrowserTest::SetUpOnMainThread();
}

void BaseLocalDataFilesBrowserTest::PreRunTestOnMainThread() {
  ExtensionBrowserTest::PreRunTestOnMainThread();
  ASSERT_TRUE(
      g_brave_browser_process->local_data_files_service()->IsInitialized());
}

void BaseLocalDataFilesBrowserTest::GetTestDataDir(
    base::FilePath* test_data_dir) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
}

void BaseLocalDataFilesBrowserTest::MaybeInitEmbeddedTestServer() {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  test_data_dir = test_data_dir.AppendASCII(embedded_test_server_directory());
  content::SetupCrossSiteRedirector(embedded_test_server());
  embedded_test_server()->ServeFilesFromDirectory(test_data_dir);
  ASSERT_TRUE(embedded_test_server()->Start());
}

void BaseLocalDataFilesBrowserTest::MaybeSetUpEmbeddedTestServerOnMainThread() {
  host_resolver()->AddRule("*", "127.0.0.1");
}

void BaseLocalDataFilesBrowserTest::WaitForService() {
  scoped_refptr<base::ThreadTestHelper> tr_helper(new base::ThreadTestHelper(
      g_brave_browser_process->local_data_files_service()->GetTaskRunner()));
  ASSERT_TRUE(tr_helper->Run());
  scoped_refptr<base::ThreadTestHelper> io_helper(new base::ThreadTestHelper(
      base::CreateSingleThreadTaskRunner({BrowserThread::IO}).get()));
  ASSERT_TRUE(io_helper->Run());
  base::RunLoop().RunUntilIdle();
}

bool BaseLocalDataFilesBrowserTest::InstallMockExtension() {
  base::FilePath test_data_dir;
  GetTestDataDir(&test_data_dir);
  const extensions::Extension* mock_extension =
      InstallExtension(test_data_dir.AppendASCII(test_data_directory()), 1);
  if (!mock_extension)
    return false;

  service()->OnComponentReady(mock_extension->id(), mock_extension->path(), "");
  WaitForService();

  return true;
}
