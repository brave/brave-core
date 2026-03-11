/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/run_until.h"
#include "brave/browser/containers/containers_service_factory.h"
#include "brave/browser/ui/browser_commands.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "brave/components/containers/core/browser/prefs.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/toolbar_button_provider.h"
#include "chrome/browser/ui/views/location_bar/icon_label_bubble_view.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/view.h"
#include "url/gurl.h"

namespace containers {

constexpr char kTestContainerId[] = "test-container-id";

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

  void SetUp() override {
    set_open_about_blank_on_browser_launch(false);
    InProcessBrowserTest::SetUp();
  }

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

  ContainersService* GetContainersService() {
    return ContainersServiceFactory::GetForProfile(browser()->profile());
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
      browser()->profile(), kContainersStoragePartitionDomain, "container-a",
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
      browser()->profile(), kContainersStoragePartitionDomain, "container-b",
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

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PRE_StoragePersistenceAcrossSessions) {
  const GURL url("https://a.test/simple.html");

  // Navigate to the page
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
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
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
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

// With the container still in the synced list, the first navigation in a
// container tab records a used snapshot via ContainerTabTracker. After the
// synced entry is removed, session restore still exposes the partition on the
// navigation entry and GetRuntimeContainerById resolves metadata from used
// prefs.
IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PRE_RuntimeContainerAvailableAfterSyncedRemovalRestart) {
  std::vector<containers::mojom::ContainerPtr> synced;
  synced.push_back(containers::mojom::Container::New(
      kTestContainerId, "ReadableName", containers::mojom::Icon::kWork,
      SK_ColorRED));
  SetContainersToPrefs(synced, *browser()->profile()->GetPrefs());

  const GURL url("https://a.test/simple.html");
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());

  const ContainersService* service = GetContainersService();
  ASSERT_TRUE(service);
  EXPECT_TRUE(service->GetRuntimeContainerById(kTestContainerId));

  PrefService* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(GetContainerFromPrefs(*prefs, kTestContainerId));
  mojom::ContainerPtr used_after_nav =
      GetUsedContainerFromPrefs(*prefs, kTestContainerId);
  ASSERT_TRUE(used_after_nav);
  EXPECT_EQ("ReadableName", used_after_nav->name);

  // Remove the container from the synced list.
  SetContainersToPrefs({}, *prefs);
  EXPECT_FALSE(GetContainerFromPrefs(*prefs, kTestContainerId));
  mojom::ContainerPtr used_after_removal =
      GetUsedContainerFromPrefs(*prefs, kTestContainerId);
  ASSERT_TRUE(used_after_removal);
  EXPECT_EQ("ReadableName", used_after_removal->name);
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       RuntimeContainerAvailableAfterSyncedRemovalRestart) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));

  content::NavigationEntry* entry =
      web_contents->GetController().GetLastCommittedEntry();
  ASSERT_TRUE(entry);
  auto storage_key = entry->GetStoragePartitionKeyToRestore();
  ASSERT_TRUE(storage_key.has_value())
      << "Restored navigation should expose container storage partition key";
  EXPECT_EQ(kContainersStoragePartitionDomain, storage_key->first);
  EXPECT_EQ(kTestContainerId, storage_key->second);

  content::StoragePartitionConfig frame_config =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition()->GetConfig();
  EXPECT_EQ(kContainersStoragePartitionDomain, frame_config.partition_domain());
  EXPECT_EQ(kTestContainerId, frame_config.partition_name());

  const ContainersService* service = GetContainersService();
  ASSERT_TRUE(service);
  mojom::ContainerPtr runtime =
      service->GetRuntimeContainerById(storage_key->second);
  ASSERT_TRUE(runtime);
  EXPECT_EQ(kTestContainerId, runtime->id);
  EXPECT_EQ("ReadableName", runtime->name);
  EXPECT_EQ(mojom::Icon::kWork, runtime->icon);
  EXPECT_EQ(SK_ColorRED, runtime->background_color);
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       LinkNavigationInheritsContainerStoragePartition) {
  const GURL url("https://a.test/simple.html");

  // Navigate to base URL with a container
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
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

  content::StoragePartition* storage_partition =
      new_tab_contents->GetPrimaryMainFrame()->GetStoragePartition();
  ASSERT_TRUE(storage_partition);
  EXPECT_EQ(kContainersStoragePartitionDomain,
            storage_partition->GetConfig().partition_domain());
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());

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
      browser()->profile(), kContainersStoragePartitionDomain, "container-a",
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
      browser()->profile(), kContainersStoragePartitionDomain, "container-b",
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

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, ShouldShowTabAccent) {
  auto* tab_strip_model = browser()->tab_strip_model();
  auto* tab_strip =
      browser()->GetBrowserView().horizontal_tab_strip_for_testing();
  ASSERT_FALSE(tab_strip->ShouldPaintTabAccent(tab_strip->tab_at(0)));

  const GURL url("https://a.test/simple.html");

  // Create a container
  auto container = containers::mojom::Container::New();
  container->id = "shared-container";
  container->name = "Shared Container";
  container->icon = containers::mojom::Icon::kSocial;
  container->background_color = SK_ColorYELLOW;

  brave::OpenUrlInContainer(browser(), url, container);
  EXPECT_EQ(2, tab_strip_model->count());

  content::WebContents* contents_in_container =
      tab_strip_model->GetActiveWebContents();
  ASSERT_TRUE(contents_in_container);
  EXPECT_TRUE(content::WaitForLoadStop(contents_in_container));

  // The tab should show accent background
  auto* tab_in_container = static_cast<BraveTab*>(
      tab_strip->tab_at(tab_strip_model->active_index()));
  EXPECT_TRUE(tab_strip->ShouldPaintTabAccent(tab_in_container));

  // The standard tab should show large accent icon
  RunScheduledLayouts();
  EXPECT_TRUE(tab_in_container->ShouldShowLargeAccentIcon());

  // A pinned tab should not show large accent icon
  tab_strip_model->SetTabPinned(tab_strip_model->active_index(), true);
  EXPECT_FALSE(tab_in_container->ShouldShowLargeAccentIcon());

  // A small unpinned tab should not show large accent icon
  tab_strip_model->SetTabPinned(tab_strip_model->active_index(), false);
  RunScheduledLayouts();

  tab_in_container->SetBounds(0, 0, 30, 30);
  EXPECT_FALSE(tab_in_container->ShouldShowLargeAccentIcon());
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
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
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
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
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

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PartitionedStorageActionIconShownOrHiddenPerTab) {
  auto* tab_strip_model = browser()->tab_strip_model();
  ASSERT_EQ(1, tab_strip_model->count());

  IconLabelBubbleView* partitioned_storage_view =
      browser()->GetBrowserView().toolbar_button_provider()->GetPageActionView(
          kActionShowPartitionedStorage);
  ASSERT_NE(nullptr, partitioned_storage_view);

  const GURL url("https://a.test/simple.html");

  // Tab 0: default (no container) -> icon should be hidden.
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  // Open a tab in a container -> icon should be visible on the active tab.
  auto container = containers::mojom::Container::New();
  container->id = "test-container";
  container->name = "Test Container";
  container->icon = containers::mojom::Icon::kSocial;
  container->background_color = SK_ColorYELLOW;

  brave::OpenUrlInContainer(browser(), url, container);
  EXPECT_EQ(2, tab_strip_model->count());
  EXPECT_TRUE(content::WaitForLoadStop(tab_strip_model->GetWebContentsAt(1)));

  EXPECT_TRUE(partitioned_storage_view->GetVisible())
      << "PartitionedStorage icon should be visible on container tab.";

  // Switch to tab 0 (default) -> icon should be hidden.
  tab_strip_model->ActivateTabAt(0);
  RunScheduledLayouts();
  EXPECT_FALSE(partitioned_storage_view->GetVisible());

  // Switch back to tab 1 (container) -> icon should be visible.
  tab_strip_model->ActivateTabAt(1);
  RunScheduledLayouts();
  EXPECT_TRUE(partitioned_storage_view->GetVisible());
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, GetStoragePartitionKeyToRestore) {
  const GURL url("https://a.test/simple.html");

  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  content::NavigationController& controller = web_contents->GetController();
  content::NavigationEntry* entry = controller.GetLastCommittedEntry();
  ASSERT_TRUE(entry);

  auto storage_key = entry->GetStoragePartitionKeyToRestore();
  ASSERT_TRUE(storage_key.has_value())
      << "NavigationEntry should have storage partition key";
  EXPECT_EQ(kContainersStoragePartitionDomain, storage_key->first);
  EXPECT_EQ(kTestContainerId, storage_key->second);
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, HotRestoreClosedContainerTab) {
  const GURL url("https://a.test/simple.html");

  // Open container tab
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* original_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(original_web_contents);

  // Set unique storage data in the container
  EXPECT_TRUE(content::ExecJs(original_web_contents,
                              SetCookieJS("reopen_cookie", "reopen_value")));
  EXPECT_TRUE(content::ExecJs(original_web_contents,
                              SetLocalStorageJS("reopen_key", "reopen_value")));

  // Close the tab
  chrome::CloseTab(browser());

  // Reopen the closed tab. The tab data is kept in-memory in this scenario.
  ui_test_utils::TabAddedWaiter wait_for_new_tab(browser());
  chrome::RestoreTab(browser());
  wait_for_new_tab.Wait();

  content::WebContents* reopened_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(reopened_web_contents);

  // Wait for navigation to complete
  EXPECT_TRUE(content::WaitForLoadStop(reopened_web_contents));

  // Verify URL is correct
  EXPECT_EQ(url, reopened_web_contents->GetLastCommittedURL());

  // Verify storage partition is restored correctly
  content::StoragePartitionConfig reopened_config =
      reopened_web_contents->GetPrimaryMainFrame()
          ->GetStoragePartition()
          ->GetConfig();
  EXPECT_EQ(kContainersStoragePartitionDomain,
            reopened_config.partition_domain());
  EXPECT_EQ(kTestContainerId, reopened_config.partition_name());

  // Verify storage data is still accessible
  content::EvalJsResult reopened_cookie_result =
      content::EvalJs(reopened_web_contents, GetCookiesJS());
  EXPECT_TRUE(reopened_cookie_result.ExtractString().find(
                  "reopen_cookie=reopen_value") != std::string::npos);
  EXPECT_EQ("reopen_value", content::EvalJs(reopened_web_contents,
                                            GetLocalStorageJS("reopen_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PRE_CrossSiteNavigationPersistence) {
  const GURL url_a("https://a.test/simple.html");
  const GURL url_b("https://b.test/simple.html");

  // Open container tab on site A
  NavigateParams params(browser(), url_a, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Set storage on site A
  EXPECT_TRUE(
      content::ExecJs(web_contents, SetCookieJS("site_a_cookie", "value_a")));
  EXPECT_TRUE(content::ExecJs(web_contents,
                              SetLocalStorageJS("site_a_key", "value_a")));

  // Navigate to site B in the same tab
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url_b));

  // Verify we're in the same container
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());

  // Set storage on site B
  EXPECT_TRUE(
      content::ExecJs(web_contents, SetCookieJS("site_b_cookie", "value_b")));
  EXPECT_TRUE(content::ExecJs(web_contents,
                              SetLocalStorageJS("site_b_key", "value_b")));

  // Verify site B storage
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find("site_b_cookie=value_b") !=
              std::string::npos);
  EXPECT_EQ("value_b",
            content::EvalJs(web_contents, GetLocalStorageJS("site_b_key")));
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, CrossSiteNavigationPersistence) {
  const GURL url_b("https://b.test/simple.html");

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Verify we restored to site B (last committed URL)
  EXPECT_EQ(url_b, web_contents->GetLastCommittedURL());

  // Verify we're in the correct container
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  EXPECT_EQ(kContainersStoragePartitionDomain,
            storage_partition->GetConfig().partition_domain());
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());

  // Verify site B storage persisted
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find("site_b_cookie=value_b") !=
              std::string::npos);
  EXPECT_EQ("value_b",
            content::EvalJs(web_contents, GetLocalStorageJS("site_b_key")));

  // Navigate back to site A using browser back
  {
    content::TestNavigationObserver nav_observer(web_contents, 1);
    ASSERT_TRUE(content::ExecJs(web_contents, "window.history.back();"));
    nav_observer.Wait();
  }

  // Verify we're still in the same container
  storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());

  // Verify site A storage is still accessible
  cookie_result = content::EvalJs(web_contents, GetCookiesJS());
  EXPECT_TRUE(cookie_result.ExtractString().find("site_a_cookie=value_a") !=
              std::string::npos);
  EXPECT_EQ("value_a",
            content::EvalJs(web_contents, GetLocalStorageJS("site_a_key")));
}

// Test navigation history persistence within a container.
// Verifies that back/forward navigation works correctly after session restore.
IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       PRE_NavigationHistoryPersistence) {
  std::vector<GURL> urls = {
      GURL("https://a.test/simple.html"),
      GURL("https://a.test/ch.html"),
  };

  // Open first page in container
  NavigateParams params(browser(), urls[0], ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, kTestContainerId,
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Navigate to page 2
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), urls[1]));

  // Verify we're on page 2
  EXPECT_EQ(urls[1], web_contents->GetLastCommittedURL());

  // Verify navigation history
  content::NavigationController& controller = web_contents->GetController();
  EXPECT_EQ(2, controller.GetEntryCount());
  EXPECT_EQ(1, controller.GetCurrentEntryIndex());  // 0-indexed
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, NavigationHistoryPersistence) {
  GURL url_page2("https://a.test/ch.html");
  GURL url_page1("https://a.test/simple.html");

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Verify we restored to page 2 (last committed)
  EXPECT_EQ(url_page2, web_contents->GetLastCommittedURL());

  // Verify we're in the correct container
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());

  // Verify navigation history was restored
  content::NavigationController& controller = web_contents->GetController();
  EXPECT_EQ(2, controller.GetEntryCount());
  EXPECT_EQ(1, controller.GetCurrentEntryIndex());

  // Navigate back to page 1
  {
    content::TestNavigationObserver nav_observer(web_contents, 1);
    ASSERT_TRUE(content::ExecJs(web_contents, "window.history.back();"));
    nav_observer.Wait();
    EXPECT_EQ(url_page1, web_contents->GetLastCommittedURL());
  }

  // Verify still in same container
  storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());

  // Navigate forward
  {
    content::TestNavigationObserver nav_observer(web_contents, 1);
    ASSERT_TRUE(content::ExecJs(web_contents, "window.history.forward();"));
    nav_observer.Wait();
    EXPECT_EQ(url_page2, web_contents->GetLastCommittedURL());
  }

  // Verify still in same container
  storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  EXPECT_EQ(kTestContainerId, storage_partition->GetConfig().partition_name());
}

// Test persistence of mixed container and non-container tabs.
// This ensures that session restore correctly handles a session with
// multiple tab types and doesn't mix up their storage partitions.
IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, PRE_MixedTabsPersistence) {
  const GURL url("https://a.test/simple.html");

  // Tab 1: Default partition (already exists from browser startup)
  content::WebContents* default_tab1 =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(
      content::ExecJs(default_tab1, SetCookieJS("default1", "value_d1")));
  EXPECT_TRUE(
      content::ExecJs(default_tab1, SetLocalStorageJS("default1", "value_d1")));

  // Tab 2: Container A
  NavigateParams params_a(browser(), url, ui::PAGE_TRANSITION_LINK);
  params_a.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params_a.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, "container-a",
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params_a);
  content::WebContents* container_a_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(
      content::ExecJs(container_a_tab, SetCookieJS("container_a", "value_a")));
  EXPECT_TRUE(content::ExecJs(container_a_tab,
                              SetLocalStorageJS("container_a", "value_a")));

  // Tab 3: Container B
  NavigateParams params_b(browser(), url, ui::PAGE_TRANSITION_LINK);
  params_b.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params_b.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, "container-b",
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params_b);
  content::WebContents* container_b_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(
      content::ExecJs(container_b_tab, SetCookieJS("container_b", "value_b")));
  EXPECT_TRUE(content::ExecJs(container_b_tab,
                              SetLocalStorageJS("container_b", "value_b")));

  // Tab 4: Another default partition tab
  NavigateParams params_d2(browser(), url, ui::PAGE_TRANSITION_LINK);
  params_d2.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  ui_test_utils::NavigateToURL(&params_d2);
  content::WebContents* default_tab2 =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(
      content::ExecJs(default_tab2, SetCookieJS("default2", "value_d2")));
  EXPECT_TRUE(
      content::ExecJs(default_tab2, SetLocalStorageJS("default2", "value_d2")));

  EXPECT_EQ(4, browser()->tab_strip_model()->count());
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest, MixedTabsPersistence) {
  // Verify all 4 tabs were restored
  EXPECT_EQ(4, browser()->tab_strip_model()->count());

  // Check each tab's storage partition and data
  for (int i = 0; i < 4; ++i) {
    content::WebContents* tab =
        browser()->tab_strip_model()->GetWebContentsAt(i);
    ASSERT_TRUE(tab);

    // Restored background tabs may not have loaded yet. Activate each tab to
    // trigger session restore's deferred loading, then wait for it to finish.
    browser()->tab_strip_model()->ActivateTabAt(i);
    ASSERT_TRUE(content::WaitForLoadStop(tab));

    content::StoragePartition* partition =
        tab->GetPrimaryMainFrame()->GetStoragePartition();
    ASSERT_TRUE(partition);

    content::EvalJsResult cookies = content::EvalJs(tab, GetCookiesJS());

    if (i == 0) {
      // Default tab 1
      EXPECT_TRUE(partition->GetConfig().is_default());
      EXPECT_TRUE(cookies.ExtractString().find("default1=value_d1") !=
                  std::string::npos);
      EXPECT_EQ("value_d1",
                content::EvalJs(tab, GetLocalStorageJS("default1")));
    } else if (i == 1) {
      // Container A
      EXPECT_EQ("container-a", partition->GetConfig().partition_name());
      EXPECT_TRUE(cookies.ExtractString().find("container_a=value_a") !=
                  std::string::npos);
      EXPECT_EQ("value_a",
                content::EvalJs(tab, GetLocalStorageJS("container_a")));
    } else if (i == 2) {
      // Container B
      EXPECT_EQ("container-b", partition->GetConfig().partition_name());
      EXPECT_TRUE(cookies.ExtractString().find("container_b=value_b") !=
                  std::string::npos);
      EXPECT_EQ("value_b",
                content::EvalJs(tab, GetLocalStorageJS("container_b")));
    } else if (i == 3) {
      // Default tab 2 - should share storage with tab 0
      EXPECT_TRUE(partition->GetConfig().is_default());
      // This tab can see cookies from both default tabs
      std::string cookie_str = cookies.ExtractString();
      EXPECT_TRUE(cookie_str.find("default1=value_d1") != std::string::npos);
      EXPECT_TRUE(cookie_str.find("default2=value_d2") != std::string::npos);
      // localStorage is origin-specific, so should have its own key
      EXPECT_EQ("value_d2",
                content::EvalJs(tab, GetLocalStorageJS("default2")));
    }
  }
}

// Test suite to verify behavior when containers feature is disabled after
// a session with container tabs.
class ContainersDisabledAfterRestoreBrowserTest : public ContainersBrowserTest {
 public:
  ContainersDisabledAfterRestoreBrowserTest() {
    const ::testing::TestInfo* test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    std::string test_name = test_info->name();

    if (!test_name.starts_with("PRE_")) {
      feature_list_override_.InitAndDisableFeature(features::kContainers);
    }
  }

 protected:
  base::test::ScopedFeatureList feature_list_override_;
};

IN_PROC_BROWSER_TEST_F(ContainersDisabledAfterRestoreBrowserTest,
                       PRE_RestoreWithDefaultPartition) {
  const GURL url("https://a.test/simple.html");

  // Navigate to the page with a container storage partition
  NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, "test-container",
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Verify we're using a container storage partition
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  ASSERT_TRUE(storage_partition);

  EXPECT_EQ(storage_partition->GetConfig().partition_domain(),
            kContainersStoragePartitionDomain);
  EXPECT_EQ(storage_partition->GetConfig().partition_name(), "test-container");

  // Set some storage data in the container
  EXPECT_TRUE(content::ExecJs(web_contents,
                              SetCookieJS("test_cookie", "container_value")));
  EXPECT_TRUE(content::ExecJs(
      web_contents, SetLocalStorageJS("test_key", "container_value")));
}

IN_PROC_BROWSER_TEST_F(ContainersDisabledAfterRestoreBrowserTest,
                       RestoreWithDefaultPartition) {
  // At this point, containers feature is disabled, but we have a restored tab
  // that was previously in a container

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Verify the URL scheme has been changed to containers+<uuid>:...
  GURL current_url = web_contents->GetLastCommittedURL();
  std::string url_scheme = std::string(current_url.scheme());

  // The URL should have a scheme like "containers+<uuid>:https"
  EXPECT_TRUE(url_scheme.starts_with("containers+")) << url_scheme;

  // Verify the storage partition is now the default one, not a container
  // partition
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  ASSERT_TRUE(storage_partition);

  content::StoragePartitionConfig default_config =
      content::StoragePartitionConfig::CreateDefault(browser()->profile());

  // The storage partition should be the default one
  EXPECT_EQ(default_config, storage_partition->GetConfig());
  EXPECT_TRUE(storage_partition->GetConfig().is_default());
  EXPECT_EQ("", storage_partition->GetConfig().partition_domain());
  EXPECT_EQ("", storage_partition->GetConfig().partition_name());

  // Verify the restored page behaves like about:blank - it's a valid page
  // but doesn't have access to storage and cookies from the container
  content::RenderFrameHost* main_frame = web_contents->GetPrimaryMainFrame();
  ASSERT_TRUE(main_frame);

  // The page is not an error document, but it's essentially blank
  EXPECT_FALSE(main_frame->IsErrorDocument());

  // Verify that JavaScript calls to access storage throw exceptions
  // (the page doesn't have a valid document context for storage APIs)
  content::EvalJsResult cookie_result =
      content::EvalJs(web_contents, GetCookiesJS());
  EXPECT_FALSE(cookie_result.is_ok())
      << "Expected JS exception when accessing cookies, but got: "
      << cookie_result;

  content::EvalJsResult local_storage_result =
      content::EvalJs(web_contents, GetLocalStorageJS("test_key"));
  EXPECT_FALSE(local_storage_result.is_ok())
      << "Expected JS exception when accessing localStorage, but got: "
      << local_storage_result;
}

IN_PROC_BROWSER_TEST_F(ContainersBrowserTest,
                       NewTabPageInheritsStoragePartitionConfig) {
  const GURL new_tab_url(chrome::kChromeUINewTabURL);

  // Open a new tab page with a container storage partition config
  NavigateParams params(browser(), new_tab_url, ui::PAGE_TRANSITION_LINK);
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  params.storage_partition_config = content::StoragePartitionConfig::Create(
      browser()->profile(), kContainersStoragePartitionDomain, "test-container",
      browser()->profile()->IsOffTheRecord());
  ui_test_utils::NavigateToURL(&params);

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(web_contents);

  // Verify the storage partition config is set correctly
  content::StoragePartition* storage_partition =
      web_contents->GetPrimaryMainFrame()->GetStoragePartition();
  ASSERT_TRUE(storage_partition);

  content::StoragePartitionConfig expected_config =
      content::StoragePartitionConfig::Create(
          browser()->profile(), kContainersStoragePartitionDomain,
          "test-container", browser()->profile()->IsOffTheRecord());

  EXPECT_EQ(expected_config, storage_partition->GetConfig());
  EXPECT_EQ("test-container", storage_partition->GetConfig().partition_name());
  EXPECT_EQ(kContainersStoragePartitionDomain,
            storage_partition->GetConfig().partition_domain());
}

}  // namespace containers
