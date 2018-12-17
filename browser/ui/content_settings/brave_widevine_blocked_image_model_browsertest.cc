/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/ui/content_settings/brave_content_setting_bubble_model.h"
#include "brave/browser/ui/content_settings/brave_widevine_blocked_image_model.h"
#include "brave/browser/ui/content_settings/brave_widevine_content_setting_bubble_model.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ssl/cert_verifier_browser_test.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "third_party/widevine/cdm/buildflags.h"

using content::WebContents;
using ImageType = ContentSettingImageModel::ImageType;

class BraveWidevineBlockedImageModelBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  std::unique_ptr<BraveWidevineBlockedImageModel> CreateModel() {
    return std::make_unique<BraveWidevineBlockedImageModel>(
        BraveWidevineBlockedImageModel::ImageType::PLUGINS,
        CONTENT_SETTINGS_TYPE_PLUGINS);
  }
};

// Tests that every model creates a valid bubble.
IN_PROC_BROWSER_TEST_F(BraveWidevineBlockedImageModelBrowserTest, CreateBubbleModel) {
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto model = CreateModel();
  std::unique_ptr<ContentSettingBubbleModel> bubble(
      model->CreateBubbleModel(nullptr, web_contents));
  ContentSettingSimpleBubbleModel* simple_bubble =
      bubble->AsSimpleBubbleModel();
  ASSERT_TRUE(simple_bubble);
  EXPECT_EQ(static_cast<ContentSettingSimpleImageModel*>(model.get())
      ->content_type(), simple_bubble->content_type());
  EXPECT_EQ(ImageType::PLUGINS, model->image_type());
}

// Tests that we correctly remember for which WebContents the animation has run,
// and thus we should not run it again.
IN_PROC_BROWSER_TEST_F(BraveWidevineBlockedImageModelBrowserTest,
                       ShouldRunAnimation) {
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto model = CreateModel();
  EXPECT_TRUE(model->ShouldRunAnimation(web_contents));
  model->SetAnimationHasRun(web_contents);
  EXPECT_FALSE(model->ShouldRunAnimation(web_contents));

  // The animation has run for the current WebContents, but not for any other.
  Profile* profile = browser()->profile();
  WebContents::CreateParams create_params(profile);
  std::unique_ptr<WebContents> other_web_contents(WebContents::Create(create_params));
  content::WebContents* raw_other_web_contents = other_web_contents.get();
  browser()->tab_strip_model()->AppendWebContents(std::move(other_web_contents), true);
  EXPECT_TRUE(model->ShouldRunAnimation(raw_other_web_contents));
}

// Tests that we go to the correct link when learn more is clicked in the bubble
IN_PROC_BROWSER_TEST_F(BraveWidevineBlockedImageModelBrowserTest,
                       LearnMoreLinkClicked) {
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  auto model = CreateModel();
  std::unique_ptr<ContentSettingBubbleModel> bubble(model->CreateBubbleModel(
      browser()->content_setting_bubble_model_delegate(), web_contents));

  content::TestNavigationObserver observer(nullptr);
  observer.StartWatchingNewWebContents();
  bubble->OnLearnMoreClicked();
  observer.Wait();

  std::string link_value(kWidevineTOS);
  EXPECT_EQ(link_value, observer.last_navigation_url().spec());
}

// Tests that the content setting model shows and that runninng plugins changes the
// opt in setting.
IN_PROC_BROWSER_TEST_F(BraveWidevineBlockedImageModelBrowserTest,
                       RunPluginsOnPageClicked) {
  GURL url = embedded_test_server()->GetURL("www.netflix.com", "/blank.html");
  ui_test_utils::NavigateToURL(browser(), url);

  auto model = CreateModel();
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  std::unique_ptr<ContentSettingBubbleModel> bubble(
          model->CreateBubbleModel(
              browser()->content_setting_bubble_model_delegate(), web_contents));

  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();

  // Before we allow, opted in should be false
  ASSERT_FALSE(prefs->GetBoolean(kWidevineOptedIn));

  ((BraveWidevineContentSettingPluginBubbleModel*)bubble.get())->RunPluginsOnPage();

  // After we allow, opted in pref should be true
  ASSERT_TRUE(prefs->GetBoolean(kWidevineOptedIn));
}

// Tests that if the user already installed and opted in, that we don't
// show the content setting model.
IN_PROC_BROWSER_TEST_F(BraveWidevineBlockedImageModelBrowserTest,
                       RunPluginsOnPageClickedAlreadyOptedIn) {
  // Start with Widevine already opted in
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  prefs->SetBoolean(kWidevineOptedIn, true);

  GURL url = embedded_test_server()->GetURL("www.netflix.com", "/blank.html");
  ui_test_utils::NavigateToURL(browser(), url);

  auto model = CreateModel();

  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  std::unique_ptr<ContentSettingBubbleModel> bubble(
          model->CreateBubbleModel(
              browser()->content_setting_bubble_model_delegate(), web_contents));

  ASSERT_FALSE(model->is_visible());
}

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
class BraveWidevineIconVisibilityBrowserTest : public CertVerifierBrowserTest {
 public:
  BraveWidevineIconVisibilityBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  void SetUpOnMainThread() override {
    CertVerifierBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    // Chromium allows the API under test only on HTTPS domains.
    base::FilePath test_data_dir;
    base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
    https_server_.ServeFilesFromDirectory(test_data_dir);
    SetUpMockCertVerifierForHttpsServer(0, net::OK);
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpDefaultCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        "enable-blink-features",
        "EncryptedMediaEncryptionSchemeQuery");
  }

  content::WebContents* active_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  bool IsWidevineIconVisible() {
    auto* location_bar = static_cast<BraveLocationBarView*>(
        browser()->window()->GetLocationBar());

    // TODO(iefremov): the hack with getting the proper view index is caused by
    // changes in brave_content_setting_image_models.cc. Probably we need to
    // refactor this to keep original indexes correct.
    ContentSettingImageView* view =
        location_bar->GetContentSettingsImageViewForTesting(
            static_cast<size_t>(
                ContentSettingImageModel::ImageType::NUM_IMAGE_TYPES)-1);
    return view->visible();
  }

 protected:
  void SetUpMockCertVerifierForHttpsServer(net::CertStatus cert_status,
                                           int net_result) {
    scoped_refptr<net::X509Certificate> cert(https_server_.GetCertificate());
    net::CertVerifyResult verify_result;
    verify_result.is_issued_by_known_root = true;
    verify_result.verified_cert = cert;
    verify_result.cert_status = cert_status;
    mock_cert_verifier()->AddResultForCert(cert, verify_result, net_result);
  }

  net::EmbeddedTestServer https_server_;
};

IN_PROC_BROWSER_TEST_F(BraveWidevineIconVisibilityBrowserTest,
                       SuggestOptInIfWidevineDetected) {
  GURL url = https_server_.GetURL("a.com", "/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(IsWidevineIconVisible());

  const std::string drm_js =
      "var config = [{initDataTypes: ['cenc']}];"
      "navigator.requestMediaKeySystemAccess($1, config);";
  const std::string widevine_js = content::JsReplace(drm_js,
                                                     "com.widevine.alpha");

  EXPECT_TRUE(content::ExecuteScript(active_contents(), widevine_js));
  EXPECT_TRUE(IsWidevineIconVisible());

  // The icon should disappear after reload.
  content::TestNavigationObserver observer(active_contents());
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  observer.Wait();
  EXPECT_FALSE(IsWidevineIconVisible());

  // Navigate to a page with some videos.
  url = https_server_.GetURL("a.com", "/media/youtube.html");
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_FALSE(IsWidevineIconVisible());

  // Check that non-widevine DRM is ignored.
  EXPECT_TRUE(
      content::ExecuteScript(active_contents(),
                             content::JsReplace(drm_js, "org.w3.clearkey")));
  EXPECT_FALSE(IsWidevineIconVisible());

  // Finally check the widevine request.
  EXPECT_TRUE(content::ExecuteScript(active_contents(), widevine_js));
  EXPECT_TRUE(IsWidevineIconVisible());
}
#endif  //  BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
