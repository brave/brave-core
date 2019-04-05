/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_theme_api.h"
#include "brave/browser/themes/brave_theme_service.h"
#include "brave/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "extensions/common/extension_builder.h"

using BTS = BraveThemeService;
using extensions::api::BraveThemeGetBraveThemeTypeFunction;
using extension_function_test_utils::RunFunctionAndReturnSingleResult;

namespace {
void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);
}
}  // namespace
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

IN_PROC_BROWSER_TEST_F(BraveThemeAPIBrowserTest,
                       BraveThemeGetBraveThemeTypeTest) {
  Profile* profile = browser()->profile();

  // Change to Light type and check it from api.
  SetBraveThemeType(profile, BraveThemeType::BRAVE_THEME_TYPE_LIGHT);
  EXPECT_EQ(BraveThemeType::BRAVE_THEME_TYPE_LIGHT,
            BTS::GetActiveBraveThemeType(profile));

  scoped_refptr<BraveThemeGetBraveThemeTypeFunction> get_function(
      new BraveThemeGetBraveThemeTypeFunction());
  get_function->set_extension(extension().get());
  std::unique_ptr<base::Value> value;
  value.reset(RunFunctionAndReturnSingleResult(get_function.get(),
                                               std::string("[]"),
                                               browser()));
  EXPECT_EQ(value->GetString(), "Light");
}
