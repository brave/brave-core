/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "brave/browser/extensions/api/brave_theme_api.h"
#include "brave/browser/extensions/brave_theme_event_router.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/extension_builder.h"
#include "testing/gmock/include/gmock/gmock.h"

using extensions::api::BraveThemeGetBraveThemeTypeFunction;
using extensions::api::BraveThemeSetBraveThemeTypeFunction;
using extension_function_test_utils::RunFunctionAndReturnSingleResult;
using BTS = BraveThemeService;

class BraveThemeAPIBrowserTest : public InProcessBrowserTest {
  public:
    void SetUpOnMainThread() override {
      InProcessBrowserTest::SetUpOnMainThread();
      extension_ = extensions::ExtensionBuilder("Test").Build();
    }

    scoped_refptr<const extensions::Extension> extension() {
      return extension_;
    }

  private:
    scoped_refptr<const extensions::Extension> extension_;
};

namespace {
class MockBraveThemeEventRouter : public extensions::BraveThemeEventRouter {
 public:
  MockBraveThemeEventRouter() {}
  ~MockBraveThemeEventRouter() override {}

  MOCK_METHOD1(OnBraveThemeTypeChanged, void(Profile*));
};

void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}
}  // namespace

IN_PROC_BROWSER_TEST_F(BraveThemeAPIBrowserTest,
                       BraveThemeGetBraveThemeTypeTest) {
  Profile* profile = browser()->profile();

  // Check default type is set initially.
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DEFAULT,
            BTS::GetUserPreferredBraveThemeType(profile));

  // Change to Light type and check it from api.
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_LIGHT,
            BTS::GetUserPreferredBraveThemeType(profile));
  scoped_refptr<BraveThemeGetBraveThemeTypeFunction> get_function(
      new BraveThemeGetBraveThemeTypeFunction());
  get_function->set_extension(extension().get());
  std::unique_ptr<base::Value> value;
  value.reset(RunFunctionAndReturnSingleResult(get_function.get(),
                                               std::string("[]"),
                                               browser()));
  EXPECT_EQ(value->GetString(), "Light");
}

IN_PROC_BROWSER_TEST_F(BraveThemeAPIBrowserTest,
                       BraveThemeSetBraveThemeTypeTest) {
  Profile* profile = browser()->profile();

  // Check default type is set initially.
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_DEFAULT,
            BTS::GetUserPreferredBraveThemeType(profile));

  // Change theme type to Light via api and check it.
  scoped_refptr<BraveThemeSetBraveThemeTypeFunction> set_function(
      new BraveThemeSetBraveThemeTypeFunction());
  set_function->set_extension(extension().get());
  RunFunctionAndReturnSingleResult(set_function.get(),
                                   std::string("[\"Light\"]"),
                                   browser());

  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_LIGHT,
            BTS::GetUserPreferredBraveThemeType(profile));
}

IN_PROC_BROWSER_TEST_F(BraveThemeAPIBrowserTest,
                       BraveThemeEventRouterTest) {
  Profile* profile = browser()->profile();
  MockBraveThemeEventRouter* mock_router_ = new MockBraveThemeEventRouter;
  EXPECT_CALL(*mock_router_, OnBraveThemeTypeChanged(profile)).Times(1);

  BraveThemeService* service = static_cast<BraveThemeService*>(
      ThemeServiceFactory::GetForProfile(browser()->profile()));
  service->SetBraveThemeEventRouterForTesting(mock_router_);
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
}
