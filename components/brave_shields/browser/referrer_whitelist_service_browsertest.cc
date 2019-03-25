/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/referrer_whitelist_service.h"
#include "chrome/browser/extensions/extension_browsertest.h"

using extensions::ExtensionBrowserTest;

const std::string kLocalDataFilesComponentTestId(
    "eclbkhjphkhalklhipiicaldjbnhdfkc");

const std::string kLocalDataFilesComponentTestBase64PublicKey =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsleoSxQ3DN+6xym2P1uX"
    "mN6ArIWd9Oru5CSjS0SRE5upM2EnAl/C20TP8JdIlPi/3tk/SN6Y92K3xIhAby5F"
    "0rbPDSTXEWGy72tv2qb/WySGwDdvYQu9/J5sEDneVcMrSHcC0VWgcZR0eof4BfOy"
    "fKMEnHX98tyA3z+vW5ndHspR/Xvo78B3+6HX6tyVm/pNlCNOm8W8feyfDfPpK2Lx"
    "qRLB7PumyhR625txxolkGC6aC8rrxtT3oymdMfDYhB4BZBrzqdriyvu1NdygoEiF"
    "WhIYw/5zv1NyIsfUiG8wIs5+OwS419z7dlMKsg1FuB2aQcDyjoXx1habFfHQfQwL"
    "qwIDAQAB";

class ReferrerWhitelistServiceTest : public ExtensionBrowserTest {
public:
  ReferrerWhitelistServiceTest() {}

  void SetUp() override {
    InitService();
    ExtensionBrowserTest::SetUp();
  }

  void PreRunTestOnMainThread() override {
    ExtensionBrowserTest::PreRunTestOnMainThread();
    WaitForReferrerWhitelistServiceThread();
    ASSERT_TRUE(g_brave_browser_process->local_data_files_service()->IsInitialized());
  }

  void InitService() {
    brave_shields::LocalDataFilesService::
        SetComponentIdAndBase64PublicKeyForTest(
            kLocalDataFilesComponentTestId,
            kLocalDataFilesComponentTestBase64PublicKey);
  }

  void GetTestDataDir(base::FilePath* test_data_dir) {
    base::ScopedAllowBlockingForTesting allow_blocking;
    brave::RegisterPathProvider();
    base::PathService::Get(brave::DIR_TEST_DATA, test_data_dir);
  }

  bool InstallReferrerWhitelistExtension() {
    base::FilePath test_data_dir;
    GetTestDataDir(&test_data_dir);
    const extensions::Extension* mock_extension =
        InstallExtension(test_data_dir.AppendASCII("referrer-whitelist-data"), 1);
    if (!mock_extension)
      return false;

    g_brave_browser_process->referrer_whitelist_service()->OnComponentReady(
      mock_extension->id(), mock_extension->path(), "");
    WaitForReferrerWhitelistServiceThread();

    return true;
  }

  void WaitForReferrerWhitelistServiceThread() {
    scoped_refptr<base::ThreadTestHelper> io_helper(
        new base::ThreadTestHelper(
            g_brave_browser_process->referrer_whitelist_service()->GetTaskRunner()));
    ASSERT_TRUE(io_helper->Run());
  }

  bool IsWhitelistedReferrer(
    const GURL& firstPartyOrigin, const GURL& subresourceUrl) {
    return g_brave_browser_process->referrer_whitelist_service()->IsWhitelisted(
      firstPartyOrigin, subresourceUrl);
  }
};

IN_PROC_BROWSER_TEST_F(ReferrerWhitelistServiceTest, IsWhitelistedReferrer) {
  ASSERT_TRUE(InstallReferrerWhitelistExtension());
  // *.fbcdn.net not allowed on some other URL
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://test.com"),
        GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // *.fbcdn.net allowed on Facebook
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
        GURL("https://video-zyz1-9.xy.fbcdn.net")));
  // Facebook doesn't allow just anything
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.facebook.com"),
        GURL("https://test.com")));
  // Allowed for reddit.com
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://www.redditmedia.com/97")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://cdn.embedly.com/157")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.reddit.com/"),
        GURL("https://imgur.com/179")));
  // Not allowed for reddit.com
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.reddit.com"),
        GURL("https://test.com")));
  // Not allowed imgur on another domain
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://www.test.com"),
        GURL("https://imgur.com/173")));
  // Fonts allowed anywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
      GURL("https://use.typekit.net/193")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://www.test.com"),
      GURL("https://cloud.typography.com/199")));
  // geetest allowed everywhere
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://binance.com"),
      GURL("https://api.geetest.com/ajax.php?")));
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("http://binance.com"),
      GURL("https://api.geetest.com/")));
  // not allowed with a different scheme
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("http://binance.com"),
      GURL("http://api.geetest.com/")));
  // Google Accounts only allows a specific hostname
  EXPECT_TRUE(IsWhitelistedReferrer(GURL("https://accounts.google.com"),
      GURL("https://content.googleapis.com/cryptauth/v1/authzen/awaittx")));
  EXPECT_FALSE(IsWhitelistedReferrer(GURL("https://accounts.google.com"),
      GURL("https://ajax.googleapis.com/ajax/libs/d3js/5.7.0/d3.min.js")));
}
