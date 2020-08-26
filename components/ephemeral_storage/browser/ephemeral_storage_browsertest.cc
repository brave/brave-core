/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/path_service.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "third_party/blink/renderer/modules/storage/brave_dom_window_storage.h"
#include "url/gurl.h"

using content::RenderFrameHost;
using net::test_server::EmbeddedTestServer;

namespace {

enum StorageType { Session, Local };

void SetStorageValueInFrame(RenderFrameHost* host,
                            std::string key,
                            std::string value,
                            StorageType storage_type) {
  std::string script = base::StringPrintf(
      "%sStorage.setItem('%s', '%s');",
      storage_type == StorageType::Session ? "session" : "local", key.c_str(),
      value.c_str());
  ASSERT_TRUE(content::ExecuteScript(host, script));
}

content::EvalJsResult GetStorageValueInFrame(RenderFrameHost* host,
                                             std::string key,
                                             StorageType storage_type) {
  std::string script = base::StringPrintf(
      "%sStorage.getItem('%s');",
      storage_type == StorageType::Session ? "session" : "local", key.c_str());
  return content::EvalJs(host, script);
}

bool NavigateRenderFrameToURL(content::RenderFrameHost* frame,
                              std::string iframe_id,
                              const GURL& url) {
  std::string script = base::StringPrintf(
      "setTimeout(\""
      "var iframes = document.getElementById('%s');iframes.src='%s';"
      "\",0)",
      iframe_id.c_str(), url.spec().c_str());

  content::TestNavigationManager navigation_manager(
      content::WebContents::FromRenderFrameHost(frame), url);
  bool result = ExecuteScript(frame, script);
  navigation_manager.WaitForNavigationFinished();
  return result;
}

}  // namespace

class EphemeralStorageBrowserTest : public InProcessBrowserTest {
 public:
  EphemeralStorageBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(blink::kBraveEphemeralStorage);
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);

    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.AddDefaultHandlers(GetChromeTestDataDir());
    content::SetupCrossSiteRedirector(&https_server_);

    ASSERT_TRUE(https_server_.Start());
    ephemeral_storage_url_ =
        https_server_.GetURL("a.com", "/ephemeral_storage.html");
    simple_url_ = https_server_.GetURL("a.com", "/simple.html");
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    // This is needed to load pages from "domain.com" without an interstitial.
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void AllowAllCookies() {
    auto* content_settings =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());
    brave_shields::SetCookieControlType(
        content_settings, brave_shields::ControlType::ALLOW, GURL());
  }

 protected:
  net::test_server::EmbeddedTestServer https_server_;
  base::test::ScopedFeatureList scoped_feature_list_;
  GURL ephemeral_storage_url_;
  GURL simple_url_;

 private:
  DISALLOW_COPY_AND_ASSIGN(EphemeralStorageBrowserTest);
};

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       StorageClearedOnMainFrameLoad) {
  AllowAllCookies();
  auto storage_types =
      std::vector<StorageType>{StorageType::Session, StorageType::Local};
  for (auto storage_type : storage_types) {
    ui_test_utils::NavigateToURL(browser(), ephemeral_storage_url_);

    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    content::RenderFrameHost* iframe =
        content::ChildFrameAt(web_contents->GetMainFrame(), 0);
    content::RenderFrameHost* third_party_iframe_a =
        content::ChildFrameAt(web_contents->GetMainFrame(), 1);
    content::RenderFrameHost* third_party_iframe_b =
        content::ChildFrameAt(web_contents->GetMainFrame(), 2);

    SetStorageValueInFrame(web_contents->GetMainFrame(), "key", "main",
                           storage_type);
    SetStorageValueInFrame(third_party_iframe_a, "key", "thirdparty",
                           storage_type);

    EXPECT_EQ("main", GetStorageValueInFrame(web_contents->GetMainFrame(),
                                             "key", storage_type));
    EXPECT_EQ("main", GetStorageValueInFrame(iframe, "key", storage_type));
    EXPECT_EQ("thirdparty", GetStorageValueInFrame(third_party_iframe_a, "key",
                                                   storage_type));
    EXPECT_EQ("thirdparty", GetStorageValueInFrame(third_party_iframe_b, "key",
                                                   storage_type));

    ui_test_utils::NavigateToURL(browser(), ephemeral_storage_url_);

    web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    iframe = content::ChildFrameAt(web_contents->GetMainFrame(), 0);
    third_party_iframe_a =
        content::ChildFrameAt(web_contents->GetMainFrame(), 1);
    third_party_iframe_b =
        content::ChildFrameAt(web_contents->GetMainFrame(), 2);

    EXPECT_EQ("main", GetStorageValueInFrame(web_contents->GetMainFrame(),
                                             "key", storage_type));
    EXPECT_EQ("main", GetStorageValueInFrame(iframe, "key", storage_type));
    EXPECT_EQ(nullptr, GetStorageValueInFrame(third_party_iframe_a, "key",
                                              storage_type));
    EXPECT_EQ(nullptr, GetStorageValueInFrame(third_party_iframe_b, "key",
                                              storage_type));
  }
}

IN_PROC_BROWSER_TEST_F(EphemeralStorageBrowserTest,
                       StoragePersistsOnSubframeNavigation) {
  AllowAllCookies();
  auto storage_types =
      std::vector<StorageType>{StorageType::Session, StorageType::Local};
  for (auto storage_type : storage_types) {
    ui_test_utils::NavigateToURL(browser(), ephemeral_storage_url_);

    auto* web_contents = browser()->tab_strip_model()->GetActiveWebContents();
    content::RenderFrameHost* main_frame = web_contents->GetMainFrame();
    content::RenderFrameHost* third_party_iframe_a =
        content::ChildFrameAt(web_contents->GetMainFrame(), 1);

    SetStorageValueInFrame(main_frame, "key", "main", storage_type);
    SetStorageValueInFrame(third_party_iframe_a, "key", "thirdparty",
                           storage_type);

    // Navigate one of the third-party iframes to a first-party URL.
    GURL third_party_url = third_party_iframe_a->GetLastCommittedURL();

    NavigateRenderFrameToURL(main_frame, "third_party_iframe_a",
                             GURL(simple_url_));
    third_party_iframe_a =
        content::ChildFrameAt(web_contents->GetMainFrame(), 1);

    // Now the value set in localStorage should reflect the first-party version.
    EXPECT_EQ("main", GetStorageValueInFrame(third_party_iframe_a, "key",
                                             storage_type));

    // Navigate back to the third-party site.
    NavigateRenderFrameToURL(main_frame, "third_party_iframe_a",
                             third_party_url);
    third_party_iframe_a =
        content::ChildFrameAt(web_contents->GetMainFrame(), 1);

    // The value set should persist in local storage since the main frame has
    // not navigated.
    EXPECT_EQ("thirdparty", GetStorageValueInFrame(third_party_iframe_a, "key",
                                                   storage_type));
  }
}
