/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/browser_commands.h"
#include "brave/components/containers/core/browser/storage_partition_constants.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/skia/include/core/SkColor.h"
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
      browser()->profile(), "default", "container-a",
      browser()->profile()->IsOffTheRecord());
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
      browser()->profile(), "default", "container-b",
      browser()->profile()->IsOffTheRecord());
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

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, IsolateServiceWorkers) {
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

  // Open a new tab with a different storage partition (container-a)
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), "default", "container-a",
      browser()->profile()->IsOffTheRecord());
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

  // Open another container (container-b)
  NavigateParams params_b(browser(), url, ui::PAGE_TRANSITION_LINK);
  params_b.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params_b.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), "default", "container-b",
      browser()->profile()->IsOffTheRecord());
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

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, OpenUrlInContainer) {
  const GURL url("https://a.test/simple.html");

  auto container = containers::mojom::Container::New();
  container->id = "test-container";
  container->name = "Test Container";
  container->icon = containers::mojom::Icon::kWork;
  container->background_color = SK_ColorBLUE;

  brave::OpenUrlInContainer(browser(), url, container);

  // Verify a new tab was created
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  // Get the newly created tab
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Verify it's the active tab
  EXPECT_EQ(1, browser()->tab_strip_model()->active_index());

  // Wait for navigation to complete
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));

  // Verify the URL is correct
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());

  // Verify the storage partition is correct
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  ASSERT_TRUE(storage_partition);

  content::StoragePartitionConfig expected_config =
      content::StoragePartitionConfig::Create(
          browser()->profile(), kContainersStoragePartitionDomain,
          "test-container", browser()->profile()->IsOffTheRecord());

  EXPECT_EQ(expected_config, storage_partition->GetConfig());

  // Set storage data in the container
  EXPECT_TRUE(
      content::ExecJs(web_contents, SetCookieJS("container_cookie", "value1")));
  EXPECT_TRUE(content::ExecJs(web_contents,
                              SetLocalStorageJS("container_key", "value1")));

  // Verify the storage data is accessible
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find("container_cookie=value1") !=
              std::string::npos);
  EXPECT_EQ("value1",
            content::EvalJs(web_contents, GetLocalStorageJS("container_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, OpenTabUrlInContainer) {
  const GURL url("https://a.test/simple.html");

  // Navigate to URL in the default (non-container) tab
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  content::WebContents* original_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(original_web_contents);

  // Set some storage data in the original tab
  EXPECT_TRUE(content::ExecJs(
      original_web_contents, SetCookieJS("original_cookie", "original_value")));
  EXPECT_TRUE(
      content::ExecJs(original_web_contents,
                      SetLocalStorageJS("original_key", "original_value")));

  // Get the tab handle
  tabs::TabInterface* tab_interface =
      browser()->tab_strip_model()->GetTabAtIndex(0);
  ASSERT_TRUE(tab_interface);
  tabs::TabHandle tab_handle = tab_interface->GetHandle();
  ASSERT_TRUE(tab_handle.Get());

  // Create a container
  auto container = containers::mojom::Container::New();
  container->id = "test-container-2";
  container->name = "Test Container 2";
  container->icon = containers::mojom::Icon::kPersonal;
  container->background_color = SK_ColorRED;

  brave::OpenTabUrlInContainer(browser(), tab_handle, container);

  // Verify a new tab was created
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  // Get the newly created tab
  content::WebContents* container_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(container_web_contents);

  // Verify it's not the same as the original
  EXPECT_NE(original_web_contents, container_web_contents);

  // Wait for navigation to complete
  EXPECT_TRUE(content::WaitForLoadStop(container_web_contents));

  // Verify the URL is correct
  EXPECT_EQ(url, container_web_contents->GetLastCommittedURL());

  // Verify the storage partition is correct
  content::StoragePartition* storage_partition =
      container_web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  ASSERT_TRUE(storage_partition);

  content::StoragePartitionConfig expected_config =
      content::StoragePartitionConfig::Create(
          browser()->profile(), kContainersStoragePartitionDomain,
          "test-container-2", browser()->profile()->IsOffTheRecord());

  EXPECT_EQ(expected_config, storage_partition->GetConfig());

  // Verify that the container tab doesn't have access to the original tab's
  // storage
  content::EvalJsResult cookie_result =
      content::EvalJs(container_web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find("original_cookie") ==
              std::string::npos);
  EXPECT_EQ(base::Value(), content::EvalJs(container_web_contents,
                                           GetLocalStorageJS("original_key")));

  // Set different storage data in the container tab
  EXPECT_TRUE(
      content::ExecJs(container_web_contents,
                      SetCookieJS("container_cookie", "container_value")));
  EXPECT_TRUE(
      content::ExecJs(container_web_contents,
                      SetLocalStorageJS("container_key", "container_value")));

  // Verify container has its own data
  cookie_result = content::EvalJs(container_web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find(
                  "container_cookie=container_value") != std::string::npos);
  EXPECT_EQ("container_value",
            content::EvalJs(container_web_contents,
                            GetLocalStorageJS("container_key")));

  // Switch back to the original tab and verify its data is unchanged
  browser()->tab_strip_model()->ActivateTabAt(0);
  content::EvalJsResult original_cookie_result =
      content::EvalJs(original_web_contents, GetCookiesJS());
  EXPECT_TRUE(original_cookie_result.ExtractString().find(
                  "original_cookie=original_value") != std::string::npos);
  EXPECT_TRUE(original_cookie_result.ExtractString().find("container_cookie") ==
              std::string::npos);
  EXPECT_EQ("original_value",
            content::EvalJs(original_web_contents,
                            GetLocalStorageJS("original_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       OpenUrlInContainer_MultipleContainers) {
  const GURL url("https://a.test/simple.html");

  // Create first container
  auto container_a = containers::mojom::Container::New();
  container_a->id = "container-a";
  container_a->name = "Container A";
  container_a->icon = containers::mojom::Icon::kWork;
  container_a->background_color = SK_ColorBLUE;

  // Create second container
  auto container_b = containers::mojom::Container::New();
  container_b->id = "container-b";
  container_b->name = "Container B";
  container_b->icon = containers::mojom::Icon::kShopping;
  container_b->background_color = SK_ColorGREEN;

  brave::OpenUrlInContainer(browser(), url, container_a);
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  content::WebContents* web_contents_a =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_a);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents_a));

  // Set storage in container A
  EXPECT_TRUE(
      content::ExecJs(web_contents_a, SetCookieJS("test_cookie", "value_a")));
  EXPECT_TRUE(content::ExecJs(web_contents_a,
                              SetLocalStorageJS("test_key", "value_a")));

  // Open URL in second container
  brave::OpenUrlInContainer(browser(), url, container_b);
  EXPECT_EQ(3, browser()->tab_strip_model()->count());

  content::WebContents* web_contents_b =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_b);
  EXPECT_NE(web_contents_a, web_contents_b);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents_b));

  // Verify container B doesn't have access to container A's storage
  content::EvalJsResult cookie_result_b =
      content::EvalJs(web_contents_b, GetCookiesJS());
  EXPECT_TRUE(cookie_result_b.ExtractString().find("test_cookie") ==
              std::string::npos);
  EXPECT_EQ(base::Value(),
            content::EvalJs(web_contents_b, GetLocalStorageJS("test_key")));

  // Set different storage in container B
  EXPECT_TRUE(
      content::ExecJs(web_contents_b, SetCookieJS("test_cookie", "value_b")));
  EXPECT_TRUE(content::ExecJs(web_contents_b,
                              SetLocalStorageJS("test_key", "value_b")));

  // Verify container B has its own data
  cookie_result_b = content::EvalJs(web_contents_b, GetCookiesJS());
  EXPECT_TRUE(cookie_result_b.ExtractString().find("test_cookie=value_b") !=
              std::string::npos);
  EXPECT_EQ("value_b",
            content::EvalJs(web_contents_b, GetLocalStorageJS("test_key")));

  // Verify container A still has its original data
  content::EvalJsResult cookie_result_a =
      content::EvalJs(web_contents_a, GetCookiesJS());
  EXPECT_TRUE(cookie_result_a.ExtractString().find("test_cookie=value_a") !=
              std::string::npos);
  EXPECT_EQ("value_a",
            content::EvalJs(web_contents_a, GetLocalStorageJS("test_key")));

  // Verify the storage partitions are different
  content::StoragePartitionConfig config_a =
      web_contents_a->GetPrimaryMainFrame()->GetStoragePartition()->GetConfig();
  content::StoragePartitionConfig config_b =
      web_contents_b->GetPrimaryMainFrame()->GetStoragePartition()->GetConfig();

  EXPECT_NE(config_a, config_b);
  EXPECT_EQ("container-a", config_a.partition_name());
  EXPECT_EQ("container-b", config_b.partition_name());
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       OpenUrlInContainer_SameContainerMultipleTabs) {
  const GURL url("https://a.test/simple.html");

  // Create a container
  auto container = containers::mojom::Container::New();
  container->id = "shared-container";
  container->name = "Shared Container";
  container->icon = containers::mojom::Icon::kSocial;
  container->background_color = SK_ColorYELLOW;

  brave::OpenUrlInContainer(browser(), url, container);
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  content::WebContents* web_contents_1 =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_1);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents_1));

  // Set storage in first tab
  EXPECT_TRUE(content::ExecJs(web_contents_1,
                              SetCookieJS("shared_cookie", "shared_value")));
  EXPECT_TRUE(content::ExecJs(web_contents_1,
                              SetLocalStorageJS("shared_key", "shared_value")));

  // Open the same URL in the same container again
  brave::OpenUrlInContainer(browser(), url, container);
  EXPECT_EQ(3, browser()->tab_strip_model()->count());

  content::WebContents* web_contents_2 =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents_2);
  EXPECT_NE(web_contents_1, web_contents_2);
  EXPECT_TRUE(content::WaitForLoadStop(web_contents_2));

  // Verify both tabs share the same storage partition
  content::StoragePartitionConfig config_1 =
      web_contents_1->GetPrimaryMainFrame()->GetStoragePartition()->GetConfig();
  content::StoragePartitionConfig config_2 =
      web_contents_2->GetPrimaryMainFrame()->GetStoragePartition()->GetConfig();

  EXPECT_EQ(config_1, config_2);
  EXPECT_EQ("shared-container", config_1.partition_name());

  // Verify second tab can access the cookie from first tab (same origin for
  // cookies)
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents_2, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find(
                  "shared_cookie=shared_value") != std::string::npos);

  // Note: localStorage is origin-specific, so web_contents_2 (b.test) won't
  // have access to web_contents_1 (a.test) localStorage, even in the same
  // container
}

}  // namespace containers
