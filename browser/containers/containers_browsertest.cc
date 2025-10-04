/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/content/browser/contained_tab_handler.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "url/gurl.h"

namespace containers {

class ContainersBrowserTest : public InProcessBrowserTest {
 public:
  ContainersBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(features::kContainers);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);

    // Register a request handler for serving test HTML content
    https_server_.AddDefaultHandlers(
        base::FilePath(FILE_PATH_LITERAL("brave/test/data")));

    EXPECT_TRUE(https_server_.Start());
  }

  ~ContainersBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        absl::StrFormat("MAP *:443 127.0.0.1:%d", https_server_.port()));
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  // JavaScript helper to set a cookie
  std::string SetCookieJS(const std::string& name, const std::string& value) {
    return absl::StrFormat(
        "document.cookie = `%s=%s; path=/; SameSite=None; Secure; expires=Wed "
        "Jan 01 2038 00:00:00 GMT`; "
        "document.cookie;",
        name, value);
  }

  // JavaScript helper to get all cookies
  std::string GetCookiesJS() { return "document.cookie;"; }

  // JavaScript helper to set localStorage item
  std::string SetLocalStorageJS(const std::string& key,
                                const std::string& value) {
    return content::JsReplace(
        "localStorage.setItem($1, $2); "
        "localStorage.getItem($1);",
        key, value);
  }

  // JavaScript helper to get localStorage item
  std::string GetLocalStorageJS(const std::string& key) {
    return content::JsReplace("localStorage.getItem($1);", key);
  }

  // JavaScript helper to set sessionStorage item
  std::string SetSessionStorageJS(const std::string& key,
                                  const std::string& value) {
    return content::JsReplace(
        "sessionStorage.setItem($1, $2); "
        "sessionStorage.getItem($1);",
        key, value);
  }

  // JavaScript helper to get sessionStorage item
  std::string GetSessionStorageJS(const std::string& key) {
    return content::JsReplace("sessionStorage.getItem($1);", key);
  }

  // JavaScript helper to set IndexedDB item
  std::string SetIndexedDBJS(const std::string& key, const std::string& value) {
    return content::JsReplace(
        "new Promise((resolve, reject) => {"
        "  const request = indexedDB.open('testDB', 1);"
        "  request.onerror = (e) => reject(e.target.error);"
        "  request.onsuccess = () => {"
        "    const db = request.result;"
        "    const transaction = db.transaction(['testStore'], 'readwrite');"
        "    const store = transaction.objectStore('testStore');"
        "    const putRequest = store.put($1, $2);"
        "    putRequest.onsuccess = () => resolve(true);"
        "    putRequest.onerror = (e) => reject(e.target.error);"
        "  };"
        "  request.onupgradeneeded = () => {"
        "    const db = request.result;"
        "    if (!db.objectStoreNames.contains('testStore')) {"
        "      db.createObjectStore('testStore');"
        "    }"
        "  };"
        "});",
        value, key);
  }

  // JavaScript helper to get IndexedDB item
  std::string GetIndexedDBJS(const std::string& key) {
    return content::JsReplace(
        "new Promise((resolve, reject) => {"
        "  const request = indexedDB.open('testDB', 1);"
        "  request.onerror = (e) => reject(e.target.error);"
        "  request.onsuccess = () => {"
        "    const db = request.result;"
        "    const transaction = db.transaction(['testStore'], 'readonly');"
        "    const store = transaction.objectStore('testStore');"
        "    const getRequest = store.get($1);"
        "    getRequest.onsuccess = () => resolve(getRequest.result || null);"
        "    getRequest.onerror = (e) => reject(e.target.error);"
        "  };"
        "  request.onupgradeneeded = () => {"
        "    const db = request.result;"
        "    if (!db.objectStoreNames.contains('testStore')) {"
        "      db.createObjectStore('testStore');"
        "    }"
        "  };"
        "});",
        key);
  }

  // JavaScript helper to clear all storage
  std::string ClearAllStorageJS() {
    return "localStorage.clear(); sessionStorage.clear(); 'cleared';";
  }

  // JavaScript helper to register a service worker
  std::string RegisterServiceWorkerJS(const std::string& script_url,
                                      const std::string& scope) {
    return content::JsReplace(
        "new Promise((resolve, reject) => {"
        "  navigator.serviceWorker.register($1, {scope: $2})"
        "    .then(registration => {"
        "      console.log('Service worker registered:', registration);"
        "      resolve('registered');"
        "    })"
        "    .catch(error => {"
        "      console.error('Service worker registration failed:', error);"
        "      reject(error.toString());"
        "    });"
        "});",
        script_url, scope);
  }

  // JavaScript helper to check if a service worker is registered
  std::string CheckServiceWorkerRegisteredJS(const std::string& scope) {
    return content::JsReplace(
        "new Promise((resolve) => {"
        "  navigator.serviceWorker.getRegistrations().then(registrations => {"
        "    const matching = registrations.filter(reg => reg.scope === $1);"
        "    resolve(matching.length > 0 ? 'registered' : 'not_registered');"
        "  });"
        "});",
        scope);
  }

  // JavaScript helper to get service worker registration count
  std::string GetServiceWorkerRegistrationCountJS() {
    return "new Promise((resolve) => {"
           "  navigator.serviceWorker.getRegistrations().then(registrations => "
           "{"
           "    resolve(registrations.length);"
           "  });"
           "});";
  }

  // JavaScript helper to unregister all service workers
  std::string UnregisterAllServiceWorkersJS() {
    return "new Promise((resolve) => {"
           "  navigator.serviceWorker.getRegistrations().then(registrations => "
           "{"
           "    const promises = registrations.map(reg => reg.unregister());"
           "    Promise.all(promises).then(() => resolve('unregistered'));"
           "  });"
           "});";
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, IsolateCookiesAndStorage) {
  const GURL url("https://a.test/simple.html");

  // Navigate to URL without container
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents_default =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_default);

  // Set storage data in the default storage partition
  EXPECT_TRUE(content::ExecJs(web_contents_default,
                              SetCookieJS("test_cookie", "value_a")));
  EXPECT_TRUE(content::ExecJs(web_contents_default,
                              SetLocalStorageJS("test_key", "value_a")));
  EXPECT_TRUE(content::ExecJs(web_contents_default,
                              SetSessionStorageJS("test_key", "value_a")));

  // Set IndexedDB data in the default storage partition
  EXPECT_TRUE(content::ExecJs(web_contents_default,
                              SetIndexedDBJS("test_key", "value_a")));

  // Verify storage data is set correctly in the default storage partition
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents_default, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find("test_cookie=value_a") !=
              std::string::npos);

  EXPECT_EQ("value_a", content::EvalJs(web_contents_default,
                                       GetLocalStorageJS("test_key")));
  EXPECT_EQ("value_a", content::EvalJs(web_contents_default,
                                       GetSessionStorageJS("test_key")));
  EXPECT_EQ("value_a",
            content::EvalJs(web_contents_default, GetIndexedDBJS("test_key")));

  // Open a new tab with a different storage partition
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container-a", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents_container_a =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_container_a);

  // Verify that the new container doesn't have access to the first container's
  // storage
  content::EvalJsResult cookie_result_a =
      content::EvalJs(web_contents_container_a, GetCookiesJS());
  EXPECT_TRUE(cookie_result_a.ExtractString().find("test_cookie=value_a") ==
              std::string::npos);

  EXPECT_EQ(base::Value(), content::EvalJs(web_contents_container_a,
                                           GetLocalStorageJS("test_key")));
  EXPECT_EQ(base::Value(), content::EvalJs(web_contents_container_a,
                                           GetSessionStorageJS("test_key")));
  EXPECT_EQ(base::Value(), content::EvalJs(web_contents_container_a,
                                           GetIndexedDBJS("test_key")));

  // Set different storage data in the container
  EXPECT_TRUE(content::ExecJs(web_contents_container_a,
                              SetCookieJS("test_cookie", "value_b")));
  EXPECT_TRUE(content::ExecJs(web_contents_container_a,
                              SetLocalStorageJS("test_key", "value_b")));
  EXPECT_TRUE(content::ExecJs(web_contents_container_a,
                              SetSessionStorageJS("test_key", "value_b")));
  EXPECT_TRUE(content::ExecJs(web_contents_container_a,
                              SetIndexedDBJS("test_key", "value_b")));

  // Verify container has its own data
  cookie_result_a = content::EvalJs(web_contents_container_a, GetCookiesJS());
  EXPECT_TRUE(cookie_result_a.ExtractString().find("test_cookie=value_b") !=
              std::string::npos);
  EXPECT_EQ("value_b", content::EvalJs(web_contents_container_a,
                                       GetLocalStorageJS("test_key")));
  EXPECT_EQ("value_b", content::EvalJs(web_contents_container_a,
                                       GetSessionStorageJS("test_key")));
  EXPECT_EQ("value_b", content::EvalJs(web_contents_container_a,
                                       GetIndexedDBJS("test_key")));

  // Check the data in default tab to container and verify its data is unchanged
  content::EvalJsResult cookie_result_default =
      content::EvalJs(web_contents_default, GetCookiesJS());
  EXPECT_TRUE(cookie_result_default.ExtractString().find(
                  "test_cookie=value_a") != std::string::npos);
  EXPECT_EQ("value_a", content::EvalJs(web_contents_default,
                                       GetLocalStorageJS("test_key")));
  EXPECT_EQ("value_a", content::EvalJs(web_contents_default,
                                       GetSessionStorageJS("test_key")));
  EXPECT_EQ("value_a",
            content::EvalJs(web_contents_default, GetIndexedDBJS("test_key")));

  // Verify default storage partition doesn't have access to container's data
  EXPECT_TRUE(cookie_result_default.ExtractString().find(
                  "test_cookie=value_b") == std::string::npos);

  // Open a new tab with a different storage partition
  NavigateParams params_b(browser(), url, ui::PAGE_TRANSITION_LINK);
  params_b.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params_b.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container-b", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params_b);

  content::WebContents* web_contents_container_b =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_container_b);

  // Verify that the new container doesn't have access to the first container's
  // storage
  content::EvalJsResult cookie_result_b =
      content::EvalJs(web_contents_container_b, GetCookiesJS());
  EXPECT_TRUE(cookie_result_b.ExtractString().find("test_cookie=value_a") ==
              std::string::npos);
  EXPECT_EQ(base::Value(), content::EvalJs(web_contents_container_b,
                                           GetLocalStorageJS("test_key")));
  EXPECT_EQ(base::Value(), content::EvalJs(web_contents_container_b,
                                           GetSessionStorageJS("test_key")));
  EXPECT_EQ(base::Value(), content::EvalJs(web_contents_container_b,
                                           GetIndexedDBJS("test_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PRE_StoragePersistenceAcrossSessions) {
  const GURL url("https://a.test/simple.html");

  // Navigate to the page
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Set persistent storage data
  EXPECT_TRUE(content::ExecJs(
      web_contents, SetCookieJS("persistent_cookie", "persistent_value")));
  EXPECT_TRUE(content::ExecJs(
      web_contents, SetLocalStorageJS("persistent_key", "persistent_value")));

  EXPECT_TRUE(content::ExecJs(
      web_contents, SetIndexedDBJS("persistent_key", "persistent_value")));

  // Verify data is set
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find(
                  "persistent_cookie=persistent_value") != std::string::npos);

  EXPECT_EQ("persistent_value",
            content::EvalJs(web_contents, GetLocalStorageJS("persistent_key")));
  EXPECT_EQ("persistent_value",
            content::EvalJs(web_contents, GetIndexedDBJS("persistent_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       StoragePersistenceAcrossSessions) {
  const GURL url("https://a.test/simple.html");

  // Navigate to the page
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents_reloaded =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_reloaded);

  // Verify persistent data is still available after reload
  content::EvalJsResult cookie_result_reloaded =
      content::EvalJs(web_contents_reloaded, GetCookiesJS());
  EXPECT_TRUE(cookie_result_reloaded.ExtractString().find(
                  "persistent_cookie=persistent_value") != std::string::npos);

  EXPECT_EQ("persistent_value",
            content::EvalJs(web_contents_reloaded,
                            GetLocalStorageJS("persistent_key")));
  EXPECT_EQ(
      "persistent_value",
      content::EvalJs(web_contents_reloaded, GetIndexedDBJS("persistent_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       LinkNavigationInheritsContainerStoragePartition) {
  const GURL url("https://a.test/simple.html");

  // Navigate to base URL with a container
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container-for-links", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* container_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(container_web_contents);

  EXPECT_TRUE(
      content::ExecJs(container_web_contents,
                      SetCookieJS("container_cookie", "container_value")));
  EXPECT_TRUE(
      content::ExecJs(container_web_contents,
                      SetLocalStorageJS("container_key", "container_value")));
  EXPECT_TRUE(
      content::ExecJs(container_web_contents,
                      SetSessionStorageJS("container_key", "container_value")));
  EXPECT_TRUE(
      content::ExecJs(container_web_contents,
                      SetIndexedDBJS("container_key", "container_value")));

  // Inject a link that opens in new tab
  std::string inject_new_tab_link_js = content::JsReplace(
      "const link = document.createElement('a');"
      "link.href = $1;"
      "link.textContent = 'New Tab Link';"
      "link.id = 'new-tab-link';"
      "link.target = '_blank';"
      "document.body.appendChild(link);"
      "link.click();",
      url);

  content::WebContentsAddedObserver new_tab_observer;
  EXPECT_TRUE(content::ExecJs(container_web_contents, inject_new_tab_link_js));
  content::WebContents* new_tab_contents = new_tab_observer.GetWebContents();
  ASSERT_TRUE(new_tab_contents);

  ASSERT_NE(new_tab_contents, container_web_contents);

  // Wait for the new tab to load
  EXPECT_TRUE(content::WaitForLoadStop(new_tab_contents));

  // Verify the new tab is on the correct URL
  EXPECT_EQ(url, new_tab_contents->GetLastCommittedURL());

  // Verify the new tab has access to the same container storage partition
  content::EvalJsResult cookie_result =
      content::EvalJs(new_tab_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find(
                  "container_cookie=container_value") != std::string::npos);
  EXPECT_EQ(
      "container_value",
      content::EvalJs(new_tab_contents, GetLocalStorageJS("container_key")));
  EXPECT_EQ(
      base::Value(),
      content::EvalJs(new_tab_contents, GetSessionStorageJS("container_key")));
  EXPECT_EQ("container_value",
            content::EvalJs(new_tab_contents, GetIndexedDBJS("container_key")));

  // Verify that setting storage in the new tab affects the container
  EXPECT_TRUE(content::ExecJs(new_tab_contents,
                              SetCookieJS("new_tab_cookie", "new_tab_value")));
  EXPECT_TRUE(content::ExecJs(
      new_tab_contents, SetLocalStorageJS("new_tab_key", "new_tab_value")));
  EXPECT_TRUE(content::ExecJs(
      new_tab_contents, SetSessionStorageJS("new_tab_key", "new_tab_value")));
  EXPECT_TRUE(content::ExecJs(new_tab_contents,
                              SetIndexedDBJS("new_tab_key", "new_tab_value")));

  // Verify the original container tab can see the new storage
  cookie_result = content::EvalJs(container_web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find(
                  "new_tab_cookie=new_tab_value") != std::string::npos);
  EXPECT_EQ("new_tab_value", content::EvalJs(container_web_contents,
                                             GetLocalStorageJS("new_tab_key")));
  EXPECT_EQ(base::Value(), content::EvalJs(container_web_contents,
                                           GetSessionStorageJS("new_tab_key")));
  EXPECT_EQ("new_tab_value", content::EvalJs(container_web_contents,
                                             GetIndexedDBJS("new_tab_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       IsolateServiceWorkersBetweenContainers) {
  const GURL url("https://a.test/containers/container_test.html");
  const GURL worker_url("https://a.test/containers/container_worker.js");
  const std::string scope = "https://a.test/containers/";

  // Navigate to URL without container
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* web_contents_default =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_default);

  // Register service worker in default storage partition
  EXPECT_TRUE(content::ExecJs(
      web_contents_default, RegisterServiceWorkerJS(worker_url.spec(), scope)));

  // Verify service worker is registered in default partition
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_default,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify we have exactly 1 service worker registration
  EXPECT_EQ(1, content::EvalJs(web_contents_default,
                               GetServiceWorkerRegistrationCountJS()));

  // Open a new tab with a different storage partition (container_a)
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container-a", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents_container_a =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_container_a);

  // Verify that the container doesn't see the service worker from default
  // partition
  EXPECT_EQ("not_registered",
            content::EvalJs(web_contents_container_a,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify container has 0 service worker registrations
  EXPECT_EQ(0, content::EvalJs(web_contents_container_a,
                               GetServiceWorkerRegistrationCountJS()));

  // Register a different service worker in the container
  EXPECT_TRUE(
      content::ExecJs(web_contents_container_a,
                      RegisterServiceWorkerJS(worker_url.spec(), scope)));

  // Verify service worker is registered in container partition
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_container_a,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify container has exactly 1 service worker registration
  EXPECT_EQ(1, content::EvalJs(web_contents_container_a,
                               GetServiceWorkerRegistrationCountJS()));

  // Verify default partition still has its service worker
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_default,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify default partition still has exactly 1 service worker registration
  EXPECT_EQ(1, content::EvalJs(web_contents_default,
                               GetServiceWorkerRegistrationCountJS()));

  // Open another container (container_b)
  NavigateParams params_b(browser(), url, ui::PAGE_TRANSITION_LINK);
  params_b.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params_b.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "container-b", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params_b);

  content::WebContents* web_contents_container_b =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_container_b);

  // Verify that container_b doesn't see service workers from other partitions
  EXPECT_EQ("not_registered",
            content::EvalJs(web_contents_container_b,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify container_b has 0 service worker registrations
  EXPECT_EQ(0, content::EvalJs(web_contents_container_b,
                               GetServiceWorkerRegistrationCountJS()));

  // Register service worker in container_b
  EXPECT_TRUE(
      content::ExecJs(web_contents_container_b,
                      RegisterServiceWorkerJS(worker_url.spec(), scope)));

  // Verify service worker is registered in container_b partition
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_container_b,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify container_b has exactly 1 service worker registration
  EXPECT_EQ(1, content::EvalJs(web_contents_container_b,
                               GetServiceWorkerRegistrationCountJS()));

  // Verify other partitions are unaffected
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_default,
                            CheckServiceWorkerRegisteredJS(scope)));
  EXPECT_EQ(1, content::EvalJs(web_contents_default,
                               GetServiceWorkerRegistrationCountJS()));

  EXPECT_EQ("registered",
            content::EvalJs(web_contents_container_a,
                            CheckServiceWorkerRegisteredJS(scope)));
  EXPECT_EQ(1, content::EvalJs(web_contents_container_a,
                               GetServiceWorkerRegistrationCountJS()));

  // Test unregistering service worker in one container doesn't affect others
  EXPECT_TRUE(content::ExecJs(web_contents_container_a,
                              UnregisterAllServiceWorkersJS()));

  // Verify container_a no longer has service workers
  EXPECT_EQ("not_registered",
            content::EvalJs(web_contents_container_a,
                            CheckServiceWorkerRegisteredJS(scope)));
  EXPECT_EQ(0, content::EvalJs(web_contents_container_a,
                               GetServiceWorkerRegistrationCountJS()));

  // Verify other partitions still have their service workers
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_default,
                            CheckServiceWorkerRegisteredJS(scope)));
  EXPECT_EQ(1, content::EvalJs(web_contents_default,
                               GetServiceWorkerRegistrationCountJS()));

  EXPECT_EQ("registered",
            content::EvalJs(web_contents_container_b,
                            CheckServiceWorkerRegisteredJS(scope)));
  EXPECT_EQ(1, content::EvalJs(web_contents_container_b,
                               GetServiceWorkerRegistrationCountJS()));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PRE_ServiceWorkerPersistenceAcrossSessions) {
  const GURL url("https://a.test/containers/container_test.html");
  const GURL worker_url("https://a.test/containers/container_worker.js");
  const std::string scope = "https://a.test/containers/";

  // Navigate to the page with a container
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "persistent-container", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Register service worker
  EXPECT_TRUE(content::ExecJs(
      web_contents, RegisterServiceWorkerJS(worker_url.spec(), scope)));

  // Verify service worker is registered
  EXPECT_EQ(
      "registered",
      content::EvalJs(web_contents, CheckServiceWorkerRegisteredJS(scope)));

  // Set some persistent storage data that the service worker might use
  EXPECT_TRUE(content::ExecJs(
      web_contents, SetLocalStorageJS("sw_data", "persistent_value")));
  EXPECT_TRUE(content::ExecJs(web_contents,
                              SetCookieJS("sw_cookie", "persistent_cookie")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       ServiceWorkerPersistenceAcrossSessions) {
  const GURL url("https://a.test/containers/container_test.html");
  const std::string scope = "https://a.test/containers/";

  // Navigate to the page with the same container
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(),
      base::StrCat({containers::ContainedTabHandler::kIdPrefix, "default"}),
      "persistent-container", browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents_reloaded =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_reloaded);

  // Verify service worker is still registered after browser restart
  EXPECT_EQ("registered",
            content::EvalJs(web_contents_reloaded,
                            CheckServiceWorkerRegisteredJS(scope)));

  // Verify persistent storage data is still available
  EXPECT_EQ("persistent_value", content::EvalJs(web_contents_reloaded,
                                                GetLocalStorageJS("sw_data")));

  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents_reloaded, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find(
                  "sw_cookie=persistent_cookie") != std::string::npos);
}

}  // namespace containers
