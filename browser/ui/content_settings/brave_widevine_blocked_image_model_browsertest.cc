/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/ui/content_settings/brave_content_setting_bubble_model.h"
#include "brave/browser/ui/content_settings/brave_widevine_blocked_image_model.h"
#include "brave/browser/ui/content_settings/brave_widevine_content_setting_bubble_model.h"
#include "brave/common/brave_paths.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_navigation_observer.h"
#include "net/dns/mock_host_resolver.h"

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

  bool NavigateToURLUntilLoadStop(const GURL& url) {
    ui_test_utils::NavigateToURL(browser(), url);
    return WaitForLoadStop(active_contents());
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
  Profile* profile = browser()->profile();
  auto model = CreateModel();
  std::unique_ptr<ContentSettingBubbleModel> bubble(
      model->CreateBubbleModel(nullptr, web_contents, profile));
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
  Profile* profile = browser()->profile();
  std::unique_ptr<ContentSettingBubbleModel> bubble(model->CreateBubbleModel(
      browser()->content_setting_bubble_model_delegate(), web_contents,
      profile));

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
  NavigateToURLUntilLoadStop(url);

  auto model = CreateModel();
  Profile* profile = browser()->profile();
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  std::unique_ptr<BraveWidevineContentSettingPluginBubbleModel> bubble(
      (BraveWidevineContentSettingPluginBubbleModel*)
          model->CreateBubbleModel(
              browser()->content_setting_bubble_model_delegate(), web_contents,
              profile));

  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();

  // Before we allow, opted in should be false
  ASSERT_FALSE(prefs->GetBoolean(kWidevineOptedIn));

  bubble->RunPluginsOnPage();

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
  NavigateToURLUntilLoadStop(url);

  auto model = CreateModel();

  Profile* profile = browser()->profile();
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  std::unique_ptr<BraveWidevineContentSettingPluginBubbleModel> bubble(
      (BraveWidevineContentSettingPluginBubbleModel*)
          model->CreateBubbleModel(
              browser()->content_setting_bubble_model_delegate(), web_contents,
              profile));

  ASSERT_FALSE(model->is_visible());
}
