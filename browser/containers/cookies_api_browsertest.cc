/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <string>

#include "base/json/string_escape.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/containers/cookie_store_id.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/containers_test_utils.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/background_script_executor.h"
#include "extensions/test/result_catcher.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/skia/include/core/SkColor.h"

namespace extensions {
namespace {

constexpr char kTestContainerId[] = "cookie-store-test-container";

class ContainerCookiesExtensionApiTest : public ExtensionApiTest {
 public:
  ContainerCookiesExtensionApiTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(containers::features::kContainers);
  }

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    ASSERT_TRUE(https_server_.Start());

    base::PathService::Get(brave::DIR_TEST_DATA, &extension_dir_);
    extension_dir_ =
        extension_dir_.AppendASCII("extensions/api_test/cookies_containers");

    containers::SetContainersEnabled(true, browser()->profile()->GetPrefs());

    auto* service =
        ContainersServiceFactory::GetForProfile(browser()->profile());
    ASSERT_TRUE(service);
    std::vector<containers::mojom::ContainerPtr> list;
    list.push_back(containers::mojom::Container::New(
        kTestContainerId, "Cookie Store Test",
        containers::mojom::Icon::kPersonal, SkColorSetRGB(0x11, 0x22, 0x33)));
    containers::SetContainersToPrefs(std::move(list),
                                     *browser()->profile()->GetPrefs());
    service->MarkContainerUsed(kTestContainerId);
  }

  content::WebContents* OpenContainerTabWithCookie() {
    const GURL url = https_server_.GetURL("a.test", "/simple.html");
    auto container = containers::mojom::Container::New(
        kTestContainerId, "Cookie Store Test",
        containers::mojom::Icon::kPersonal, SkColorSetRGB(0x11, 0x22, 0x33));
    brave::OpenUrlInContainer(browser(), url, container);
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_TRUE(web_contents);
    EXPECT_TRUE(content::WaitForLoadStop(web_contents));
    EXPECT_TRUE(content::ExecJs(
        web_contents,
        "document.cookie = 'container_cookie=from_page; path=/; Secure; "
        "SameSite=None';"));
    return web_contents;
  }

  const Extension* LoadCookiesTestExtension() {
    const Extension* extension = LoadExtension(extension_dir_);
    EXPECT_TRUE(extension);
    return extension;
  }

  bool RunBackgroundScript(const Extension& extension,
                           const std::string& script) {
    ResultCatcher catcher;
    if (!BackgroundScriptExecutor::ExecuteScriptAsync(
            browser()->profile(), extension.id(), script,
            browsertest_util::ScriptUserActivation::kDontActivate)) {
      return false;
    }
    return catcher.GetNextResult();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;
  base::FilePath extension_dir_;
};

// Verifies the real chrome.cookies surface: getAllCookieStores exposes
// containers:<uuid>, getAll({storeId}) returns container cookies, and the
// default store (omit storeId / "0") does not.
IN_PROC_BROWSER_TEST_F(ContainerCookiesExtensionApiTest,
                       GetAllCookieStoresAndGetAll_ViaChromeCookies) {
  content::WebContents* web_contents = OpenContainerTabWithCookie();
  ASSERT_TRUE(web_contents);

  const int tab_id = ExtensionTabUtil::GetTabId(web_contents);
  const std::string expected_store_id =
      containers::GetContainerStoreId(kTestContainerId);
  const GURL url = web_contents->GetLastCommittedURL();

  const Extension* extension = LoadCookiesTestExtension();
  ASSERT_TRUE(extension);

  const std::string script = base::StringPrintf(
      R"(
      (async () => {
        try {
          const expectedStoreId = %s;
          const tabId = %d;
          const url = %s;

          const stores = await chrome.cookies.getAllCookieStores();
          const containerStore = stores.find(s => s.id === expectedStoreId);
          if (!containerStore) {
            chrome.test.fail(
                'Missing container store. stores=' + JSON.stringify(stores));
            return;
          }
          if (!containerStore.tabIds.includes(tabId)) {
            chrome.test.fail(
                'Container store missing tabId. store=' +
                JSON.stringify(containerStore));
            return;
          }

          const containerCookies = await chrome.cookies.getAll({
            storeId: expectedStoreId,
            url,
          });
          const match = containerCookies.find(
              c => c.name === 'container_cookie' && c.value === 'from_page');
          if (!match) {
            chrome.test.fail(
                'Container cookie missing. cookies=' +
                JSON.stringify(containerCookies));
            return;
          }
          if (match.storeId !== expectedStoreId) {
            chrome.test.fail('Wrong storeId on cookie: ' + match.storeId);
            return;
          }

          const defaultCookies = await chrome.cookies.getAll({url});
          if (defaultCookies.some(c => c.name === 'container_cookie')) {
            chrome.test.fail(
                'Default getAll unexpectedly saw container cookie: ' +
                JSON.stringify(defaultCookies));
            return;
          }

          const storeZeroCookies =
              await chrome.cookies.getAll({storeId: '0', url});
          if (storeZeroCookies.some(c => c.name === 'container_cookie')) {
            chrome.test.fail(
                'storeId 0 unexpectedly saw container cookie: ' +
                JSON.stringify(storeZeroCookies));
            return;
          }

          chrome.test.succeed();
        } catch (e) {
          chrome.test.fail(String(e));
        }
      })();
      )",
      base::GetQuotedJSONString(expected_store_id).c_str(), tab_id,
      base::GetQuotedJSONString(url.spec()).c_str());

  EXPECT_TRUE(RunBackgroundScript(*extension, script)) << message_;
}

// Verifies set/remove against a container store via chrome.cookies.
IN_PROC_BROWSER_TEST_F(ContainerCookiesExtensionApiTest,
                       SetAndRemove_ViaChromeCookies) {
  content::WebContents* web_contents = OpenContainerTabWithCookie();
  ASSERT_TRUE(web_contents);

  const std::string expected_store_id =
      containers::GetContainerStoreId(kTestContainerId);
  const GURL url = web_contents->GetLastCommittedURL();

  const Extension* extension = LoadCookiesTestExtension();
  ASSERT_TRUE(extension);

  const std::string script = base::StringPrintf(
      R"(
      (async () => {
        try {
          const expectedStoreId = %s;
          const url = %s;

          await chrome.cookies.set({
            url,
            name: 'ext_set_cookie',
            value: 'from_extension',
            storeId: expectedStoreId,
            secure: true,
            sameSite: 'no_restriction',
          });

          let cookies = await chrome.cookies.getAll({
            storeId: expectedStoreId,
            url,
            name: 'ext_set_cookie',
          });
          if (cookies.length !== 1 || cookies[0].value !== 'from_extension') {
            chrome.test.fail('set failed: ' + JSON.stringify(cookies));
            return;
          }

          await chrome.cookies.remove({
            url,
            name: 'ext_set_cookie',
            storeId: expectedStoreId,
          });

          cookies = await chrome.cookies.getAll({
            storeId: expectedStoreId,
            url,
            name: 'ext_set_cookie',
          });
          if (cookies.length !== 0) {
            chrome.test.fail('remove failed: ' + JSON.stringify(cookies));
            return;
          }

          chrome.test.succeed();
        } catch (e) {
          chrome.test.fail(String(e));
        }
      })();
      )",
      base::GetQuotedJSONString(expected_store_id).c_str(),
      base::GetQuotedJSONString(url.spec()).c_str());

  EXPECT_TRUE(RunBackgroundScript(*extension, script)) << message_;
}

// Invalid container store ids must fail through the real API.
IN_PROC_BROWSER_TEST_F(ContainerCookiesExtensionApiTest,
                       InvalidStoreId_ViaChromeCookies) {
  const Extension* extension = LoadCookiesTestExtension();
  ASSERT_TRUE(extension);

  constexpr char kScript[] = R"(
    (async () => {
      try {
        await chrome.cookies.getAll({storeId: 'containers:does-not-exist'});
        chrome.test.fail('Expected invalid storeId to reject');
      } catch (e) {
        chrome.test.succeed();
      }
    })();
  )";

  EXPECT_TRUE(RunBackgroundScript(*extension, kScript)) << message_;
}

// tabs.Tab.cookieStoreId matches the container cookies storeId and can be
// passed directly to chrome.cookies.getAll.
IN_PROC_BROWSER_TEST_F(ContainerCookiesExtensionApiTest,
                       TabsCookieStoreId_MatchesCookiesStore) {
  content::WebContents* web_contents = OpenContainerTabWithCookie();
  ASSERT_TRUE(web_contents);

  const int tab_id = ExtensionTabUtil::GetTabId(web_contents);
  const std::string expected_store_id =
      containers::GetContainerStoreId(kTestContainerId);
  const GURL url = web_contents->GetLastCommittedURL();

  const Extension* extension = LoadCookiesTestExtension();
  ASSERT_TRUE(extension);

  const std::string script = base::StringPrintf(
      R"(
      (async () => {
        try {
          const expectedStoreId = %s;
          const tabId = %d;
          const url = %s;

          const tab = await chrome.tabs.get(tabId);
          if (tab.cookieStoreId !== expectedStoreId) {
            chrome.test.fail(
                'tab.cookieStoreId mismatch: ' + tab.cookieStoreId +
                ' expected ' + expectedStoreId);
            return;
          }

          const cookies = await chrome.cookies.getAll({
            url,
            storeId: tab.cookieStoreId,
          });
          const match = cookies.find(
              c => c.name === 'container_cookie' && c.value === 'from_page');
          if (!match) {
            chrome.test.fail(
                'cookies via tab.cookieStoreId missing. cookies=' +
                JSON.stringify(cookies));
            return;
          }

          chrome.test.succeed();
        } catch (e) {
          chrome.test.fail(String(e));
        }
      })();
      )",
      base::GetQuotedJSONString(expected_store_id).c_str(), tab_id,
      base::GetQuotedJSONString(url.spec()).c_str());

  EXPECT_TRUE(RunBackgroundScript(*extension, script)) << message_;
}

// Default (non-container) tabs report cookieStoreId "0".
IN_PROC_BROWSER_TEST_F(ContainerCookiesExtensionApiTest,
                       TabsCookieStoreId_DefaultIsZero) {
  const GURL url = https_server_.GetURL("a.test", "/simple.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  const int tab_id = ExtensionTabUtil::GetTabId(web_contents);

  const Extension* extension = LoadCookiesTestExtension();
  ASSERT_TRUE(extension);

  const std::string script = base::StringPrintf(
      R"(
      (async () => {
        try {
          const tab = await chrome.tabs.get(%d);
          if (tab.cookieStoreId !== '0') {
            chrome.test.fail(
                'Expected cookieStoreId \"0\", got ' + tab.cookieStoreId);
            return;
          }
          chrome.test.succeed();
        } catch (e) {
          chrome.test.fail(String(e));
        }
      })();
      )",
      tab_id);

  EXPECT_TRUE(RunBackgroundScript(*extension, script)) << message_;
}

}  // namespace
}  // namespace extensions
