/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/themes/theme_service_factory.h"
#endif

class BraveThemeSourceTest : public testing::Test {
 protected:
  void SetUp() override {
    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
  }

  // Returns what `source` serves for chrome://theme/`path`, requested from
  // `web_contents`, or nullopt if it serves nothing.
  std::optional<std::string> GetData(ThemeSource& source,
                                     const std::string& path,
                                     content::WebContents* web_contents) {
    std::optional<std::string> result;
    base::RunLoop run_loop;
    source.StartDataRequest(
        GURL(base::StrCat({content::kChromeUIScheme, "://",
                           chrome::kChromeUIThemeHost, "/", path})),
        base::BindLambdaForTesting([&] { return web_contents; }),
        base::BindLambdaForTesting(
            [&](scoped_refptr<base::RefCountedMemory> data) {
              if (data) {
                result.emplace(data->begin(), data->end());
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return result;
  }

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
};

#if !BUILDFLAG(IS_ANDROID)
// Brave loads the ThemeSource in the system profile, which has no
// ThemeService, so colors.css is served empty there instead of crashing.
TEST_F(BraveThemeSourceTest, SystemProfileServesNoColorsCss) {
  Profile* system_profile = profile_manager_->CreateSystemProfile();
  ASSERT_FALSE(
      ThemeServiceFactory::GetForProfile(system_profile->GetOriginalProfile()));
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(system_profile,
                                                        /*instance=*/nullptr);
  ThemeSource source(system_profile);

  EXPECT_EQ(GetData(source, "colors.css?sets=ui", web_contents.get()),
            std::nullopt);
}
#endif  // !BUILDFLAG(IS_ANDROID)

// A regular profile still gets its colors.css.
TEST_F(BraveThemeSourceTest, RegularProfileServesColorsCss) {
  Profile* profile = profile_manager_->CreateTestingProfile("profile");
  std::unique_ptr<content::WebContents> web_contents =
      content::WebContentsTester::CreateTestWebContents(profile,
                                                        /*instance=*/nullptr);
  ThemeSource source(profile);

  std::optional<std::string> css =
      GetData(source, "colors.css?sets=ui", web_contents.get());
  ASSERT_TRUE(css);
  EXPECT_FALSE(css->empty());
}
