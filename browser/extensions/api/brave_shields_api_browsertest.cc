/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/path_service.h"
#include "brave/browser/extensions/api/brave_shields_api.h"
#include "brave/common/brave_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "extensions/common/extension_builder.h"
#include "net/dns/mock_host_resolver.h"

using extensions::api::BraveShieldsAllowScriptsOnceFunction;
using extension_function_test_utils::RunFunctionAndReturnError;
using extension_function_test_utils::RunFunctionAndReturnSingleResult;

class BraveShieldsAPIBrowserTest : public InProcessBrowserTest {
  public:
    void SetUpOnMainThread() override {
      InProcessBrowserTest::SetUpOnMainThread();
      host_resolver()->AddRule("*", "127.0.0.1");
      content::SetupCrossSiteRedirector(embedded_test_server());

      brave::RegisterPathProvider();
      base::FilePath test_data_dir;
      base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
      embedded_test_server()->ServeFilesFromDirectory(test_data_dir);

      ASSERT_TRUE(embedded_test_server()->Start());
      extension_ = extensions::ExtensionBuilder("Test").Build();
    }

    content::WebContents* active_contents() {
      return browser()->tab_strip_model()->GetActiveWebContents();
    }

    scoped_refptr<extensions::Extension> extension() {
      return extension_;
    }

    void BlockScripts() {
      HostContentSettingsMap* content_settings =
        HostContentSettingsMapFactory::GetForProfile(browser()->profile());
      content_settings->SetContentSettingCustomScope(
          ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
          CONTENT_SETTINGS_TYPE_JAVASCRIPT, "", CONTENT_SETTING_BLOCK);
    }

    bool NavigateToURLUntilLoadStop(
        const std::string& origin, const std::string& path) {
      ui_test_utils::NavigateToURL(
          browser(),
          embedded_test_server()->GetURL(origin, path));

      return WaitForLoadStop(active_contents());
    }

  private:
    scoped_refptr<extensions::Extension> extension_;
};

IN_PROC_BROWSER_TEST_F(BraveShieldsAPIBrowserTest, AllowScriptsOnce) {
  BlockScripts();

  EXPECT_TRUE(
      NavigateToURLUntilLoadStop("a.com", "/load_js_from_origins.html"));
  EXPECT_EQ(active_contents()->GetAllFrames().size(), 1u) <<
    "All script loadings should be blocked.";

  // run extension function to temporarily allow a.com
  scoped_refptr<BraveShieldsAllowScriptsOnceFunction> function(
      new BraveShieldsAllowScriptsOnceFunction());
  function->set_extension(extension().get());
  function->set_has_callback(true);

  const GURL url(embedded_test_server()->GetURL("a.com", "/simple.js"));
  const std::string allow_origin = url.GetOrigin().spec();
  int tabId = extensions::ExtensionTabUtil::GetTabId(active_contents());

  RunFunctionAndReturnSingleResult(
    function.get(),
    "[[\"" + allow_origin + "\"], " + std::to_string(tabId) + "]",
    browser());

  // reload page with a.com temporarily allowed
  active_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                            true);
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(active_contents()->GetAllFrames().size(), 2u) <<
    "Scripts from a.com should be temporarily allowed.";

  // reload page again
  active_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                            true);
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(active_contents()->GetAllFrames().size(), 2u) <<
    "Scripts from a.com should be temporarily allowed after reload.";

  // same doc navigation
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("a.com", "/load_js_from_origins.html#foo"));
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(active_contents()->GetAllFrames().size(), 2u) <<
    "Scripts from a.com should be temporarily allowed for same doc navigation.";

  // navigate to a different origin
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("b.com", "/load_js_from_origins.html"));
  EXPECT_TRUE(WaitForLoadStop(active_contents()));
  EXPECT_EQ(active_contents()->GetAllFrames().size(), 1u) <<
    "All script loadings should be blocked after navigating away.";
}
