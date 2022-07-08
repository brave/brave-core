/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/thread_test_helper.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/extensions/brave_base_local_data_files_browsertest.h"
#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/brave_shields/browser/brave_farbling_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/constants/pref_names.h"
#include "brave/third_party/blink/renderer/brave_font_whitelist.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/language/core/browser/pref_names.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/permissions/permission_request.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/navigator_language.h"

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "brave/renderer/brave_content_renderer_client.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/content_settings/renderer/content_settings_agent_impl.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "content/public/test/render_view_test.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_element.h"

using brave_shields::ControlType;
using brave_shields::features::kBraveReduceLanguage;
using content::TitleWatcher;

namespace {
const char kNavigatorLanguagesScript[] = "navigator.languages.toString()";
const uint64_t kTestingSessionToken = 12345;
}  // namespace

class BraveNavigatorLanguagesFarblingBrowserTest : public InProcessBrowserTest {
 public:
  BraveNavigatorLanguagesFarblingBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitAndEnableFeature(kBraveReduceLanguage);
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.RegisterRequestMonitor(base::BindRepeating(
        &BraveNavigatorLanguagesFarblingBrowserTest::MonitorHTTPRequest,
        base::Unretained(this)));
    EXPECT_TRUE(https_server_.Start());
  }

  BraveNavigatorLanguagesFarblingBrowserTest(
      const BraveNavigatorLanguagesFarblingBrowserTest&) = delete;
  BraveNavigatorLanguagesFarblingBrowserTest& operator=(
      const BraveNavigatorLanguagesFarblingBrowserTest&) = delete;

  ~BraveNavigatorLanguagesFarblingBrowserTest() override {}

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    content_client_.reset(new ChromeContentClient);
    content::SetContentClient(content_client_.get());
    browser_content_client_.reset(new BraveContentBrowserClient());
    content::SetBrowserClientForTesting(browser_content_client_.get());
    g_brave_browser_process->brave_farbling_service()
        ->set_session_tokens_for_testing(kTestingSessionToken,
                                         kTestingSessionToken);

    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void TearDown() override {
    browser_content_client_.reset();
    content_client_.reset();
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  net::EmbeddedTestServer https_server_;

  HostContentSettingsMap* content_settings() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void AllowFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::ALLOW,
        https_server_.GetURL(domain, "/"));
  }

  void BlockFingerprinting(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::BLOCK,
        https_server_.GetURL(domain, "/"));
  }

  void SetFingerprintingDefault(std::string domain) {
    brave_shields::SetFingerprintingControlType(
        content_settings(), ControlType::DEFAULT,
        https_server_.GetURL(domain, "/"));
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    return WaitForLoadStop(web_contents());
  }

  void SetAcceptLanguages(const std::string& accept_languages) {
    content::BrowserContext* context =
        static_cast<content::BrowserContext*>(browser()->profile());
    PrefService* prefs = user_prefs::UserPrefs::Get(context);
    prefs->Set(language::prefs::kSelectedLanguages,
               base::Value(accept_languages));
  }

  void MonitorHTTPRequest(const net::test_server::HttpRequest& request) {
    if (request.relative_url != "/simple.html")
      return;
    if (expected_http_accept_language_.empty())
      return;
    EXPECT_EQ(request.headers.at("accept-language"),
              expected_http_accept_language_);
  }

  void SetExpectedHTTPAcceptLanguage(
      const std::string& expected_http_accept_language) {
    expected_http_accept_language_ = expected_http_accept_language;
  }

 private:
  std::unique_ptr<ChromeContentClient> content_client_;
  std::unique_ptr<BraveContentBrowserClient> browser_content_client_;
  std::string expected_http_accept_language_;
};

// Tests results of farbling known values
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleLanguages) {
  std::string domain1 = "b.test";
  std::string domain2 = "d.test";
  GURL url1 = https_server_.GetURL(domain1, "/simple.html");
  GURL url2 = https_server_.GetURL(domain2, "/simple.html");
  // Farbling level: off
  AllowFingerprinting(domain1);
  NavigateToURLUntilLoadStop(url1);
  std::string testing_languages = "en-US,en,es,la";
  SetAcceptLanguages(testing_languages);
  EXPECT_EQ(testing_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
  AllowFingerprinting(domain2);
  NavigateToURLUntilLoadStop(url2);
  EXPECT_EQ(testing_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));

  // Farbling level: default
  SetFingerprintingDefault(domain1);
  NavigateToURLUntilLoadStop(url1);
  std::string standard_languages = "en-US";
  EXPECT_EQ(standard_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
  SetFingerprintingDefault(domain2);
  NavigateToURLUntilLoadStop(url2);
  EXPECT_EQ(standard_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));

  // Farbling level: maximum
  BlockFingerprinting(domain1);
  NavigateToURLUntilLoadStop(url1);
  std::string strict_languages = "en-US,en";
  EXPECT_EQ(strict_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
  BlockFingerprinting(domain2);
  NavigateToURLUntilLoadStop(url2);
  EXPECT_EQ(strict_languages,
            EvalJs(web_contents(), kNavigatorLanguagesScript));
}

// Tests that web workers inherit the farbled navigator.languages
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleLanguagesWebWorker) {
  std::u16string expected_title(u"pass");
  std::string domain = "b.test";
  GURL url = https_server_.GetURL(domain, "/navigator/workers-languages.html");

  // Farbling level: off
  AllowFingerprinting(domain);
  NavigateToURLUntilLoadStop(url);
  TitleWatcher watcher1(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher1.WaitAndGetTitle());

  // Farbling level: default
  SetFingerprintingDefault(domain);
  NavigateToURLUntilLoadStop(url);
  TitleWatcher watcher2(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());

  // Farbling level: maximum
  BlockFingerprinting(domain);
  NavigateToURLUntilLoadStop(url);
  TitleWatcher watcher3(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher3.WaitAndGetTitle());
}

// Tests that service workers inherit the farbled navigator.languages
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleLanguagesServiceWorker) {
  std::u16string expected_title(u"pass");
  std::string domain = "b.test";
  GURL url =
      https_server_.GetURL(domain, "/navigator/service-workers-languages.html");
  // Farbling level: default
  SetFingerprintingDefault(domain);
  NavigateToURLUntilLoadStop(url);
  TitleWatcher watcher2(web_contents(), expected_title);
  EXPECT_EQ(expected_title, watcher2.WaitAndGetTitle());
}

// Tests results of farbling user agent
IN_PROC_BROWSER_TEST_F(BraveNavigatorLanguagesFarblingBrowserTest,
                       FarbleHTTPAcceptLanguage) {
  std::string domain_b = "b.test";
  std::string domain_d = "d.test";
  GURL url_b = https_server_.GetURL(domain_b, "/simple.html");
  GURL url_d = https_server_.GetURL(domain_d, "/simple.html");
  SetAcceptLanguages("la,es,en");

  // Farbling level: off
  // HTTP Accept-Language header should not be farbled.
  AllowFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("la,es;q=0.9,en;q=0.8");
  NavigateToURLUntilLoadStop(url_b);
  AllowFingerprinting(domain_d);
  NavigateToURLUntilLoadStop(url_d);

  // Farbling level: default
  // HTTP Accept-Language header should be farbled by domain.
  SetFingerprintingDefault(domain_b);
  SetExpectedHTTPAcceptLanguage("la;q=0.7");
  NavigateToURLUntilLoadStop(url_b);
  SetExpectedHTTPAcceptLanguage("la;q=0.8");
  SetFingerprintingDefault(domain_d);
  NavigateToURLUntilLoadStop(url_d);

  // Farbling level: maximum
  // HTTP Accept-Language header should be farbled but the same across domains.
  BlockFingerprinting(domain_b);
  SetExpectedHTTPAcceptLanguage("en-US,en;q=0.9");
  NavigateToURLUntilLoadStop(url_b);
  BlockFingerprinting(domain_d);
  NavigateToURLUntilLoadStop(url_d);
}

namespace content_settings {
namespace {

class MockContentSettingsManagerImpl : public mojom::ContentSettingsManager {
 public:
  struct Log {
    int on_content_blocked_count = 0;
    ContentSettingsType on_content_blocked_type = ContentSettingsType::DEFAULT;
  };

  explicit MockContentSettingsManagerImpl(Log* log) : log_(log) {}
  ~MockContentSettingsManagerImpl() override = default;

  // mojom::ContentSettingsManager methods:
  void Clone(
      mojo::PendingReceiver<mojom::ContentSettingsManager> receiver) override {
    ADD_FAILURE() << "Not reached";
  }

  void AllowStorageAccess(int32_t render_frame_id,
                          StorageType storage_type,
                          const url::Origin& origin,
                          const ::net::SiteForCookies& site_for_cookies,
                          const url::Origin& top_frame_origin,
                          base::OnceCallback<void(bool)> callback) override {}

  void AllowEphemeralStorageAccess(
      int32_t render_frame_id,
      const ::url::Origin& origin,
      const ::net::SiteForCookies& site_for_cookies,
      const ::url::Origin& top_frame_origin,
      AllowEphemeralStorageAccessCallback callback) override {}

  void OnContentBlocked(int32_t render_frame_id,
                        ContentSettingsType type) override {
    ++log_->on_content_blocked_count;
    log_->on_content_blocked_type = type;
  }

 private:
  raw_ptr<Log> log_ = nullptr;
};

class MockContentSettingsAgentImpl : public BraveContentSettingsAgentImpl {
 public:
  explicit MockContentSettingsAgentImpl(content::RenderFrame* render_frame);
  MockContentSettingsAgentImpl(const MockContentSettingsAgentImpl&) = delete;
  MockContentSettingsAgentImpl& operator=(const MockContentSettingsAgentImpl&) =
      delete;
  ~MockContentSettingsAgentImpl() override {}

  // ContentSettingAgentImpl methods:
  void BindContentSettingsManager(
      mojo::Remote<mojom::ContentSettingsManager>* manager) override;

  bool IsReduceLanguageEnabled() override {
    LOG(ERROR) << "IsReduceLanguageEnabled";
    // calling from here does nothing
    brave::set_allowed_font_families_for_testing(
        true,
        base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{}));
    return true;
  }
  int on_content_blocked_count() const { return log_.on_content_blocked_count; }
  ContentSettingsType on_content_blocked_type() const {
    return log_.on_content_blocked_type;
  }

 private:
  MockContentSettingsManagerImpl::Log log_;
};

MockContentSettingsAgentImpl::MockContentSettingsAgentImpl(
    content::RenderFrame* render_frame)
    : BraveContentSettingsAgentImpl(
          render_frame,
          false,
          std::make_unique<ContentSettingsAgentImpl::Delegate>()) {}

void MockContentSettingsAgentImpl::BindContentSettingsManager(
    mojo::Remote<mojom::ContentSettingsManager>* manager) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<MockContentSettingsManagerImpl>(&log_),
      manager->BindNewPipeAndPassReceiver());
}
}  // namespace

class FontLocalSourceFarblingTest : public content::RenderViewTest {
 public:
  FontLocalSourceFarblingTest() {
    feature_list_.InitAndEnableFeature(kBraveReduceLanguage);
  }
  ~FontLocalSourceFarblingTest() override = default;

  void SetUp() override {
    RenderViewTest::SetUp();

    // Set up a fake url loader factory to ensure that script loader can create
    // a WebURLLoader.
    CreateFakeWebURLLoaderFactory();

    // Unbind the ContentSettingsAgent interface that would be registered by
    // the ContentSettingsAgentImpl created when the render frame is created.
    GetMainRenderFrame()->GetAssociatedInterfaceRegistry()->RemoveInterface(
        mojom::ContentSettingsAgent::Name_);
  }

  content::ContentBrowserClient* CreateContentBrowserClient() override {
    return new BraveContentBrowserClient();
  }

#if 0
    content::ContentRendererClient* CreateContentRendererClient() override {
    return new BraveContentRendererClient();
  }
#endif

 private:
  base::test::ScopedFeatureList feature_list_;
};

const char kFontLocalSourceHTML[] =
    "<html><head><style>@font-face{font-family:Helvetica "
    "Shadow;src:local('Helvetica')}</style></head><body><p id='test1' "
    "style=\"font-family: 'Helvetica'\">mmMwWLliI0fiflO&1</p><p id='test2' "
    "style=\"font-family: 'Helvetica "
    "Shadow'\">mmMwWLliI0fiflO&1</p></body></html>";
TEST_F(FontLocalSourceFarblingTest, Foo) {
  // calling from here does nothing
  brave::set_allowed_font_families_for_testing(
      true,
      base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{}));
  MockContentSettingsAgentImpl agent(GetMainRenderFrame());
  LoadHTMLWithUrlOverride(kFontLocalSourceHTML,
                          GURL("http://a.test/").spec().c_str());
  blink::WebDocument document = GetMainFrame()->GetDocument();
  blink::WebElement p1 =
      document.GetElementById(blink::WebString::FromUTF8("test1"));
  ASSERT_FALSE(p1.IsNull());
  LOG(ERROR) << p1.GetComputedValue("font-family")
                    .Utf8();  // BoundsInViewport().width();
  blink::WebElement p2 =
      document.GetElementById(blink::WebString::FromUTF8("test2"));
  ASSERT_FALSE(p2.IsNull());
  LOG(ERROR) << p2.GetComputedValue("font-family")
                    .Utf8();  // BoundsInViewport().width();
  EXPECT_TRUE(0 == 1);
}

}  // namespace content_settings

#if 0
brave::set_allowed_font_families_for_testing(
    brave::CanRestrictFontFamiliesOnThisPlatform(),
    base::MakeFlatSet<base::StringPiece>(std::vector<base::StringPiece>{}));
#endif
