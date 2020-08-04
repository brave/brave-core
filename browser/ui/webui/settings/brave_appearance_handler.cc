/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/components/binance/browser/buildflags/buildflags.h"
#include "brave/components/brave_together/buildflags/buildflags.h"
#include "brave/components/crypto_exchange/browser/buildflags/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"

#if BUILDFLAG(CRYPTO_EXCHANGES_ENABLED)
#include "brave/components/crypto_exchange/browser/crypto_exchange_region_util.h"
#endif

#if BUILDFLAG(BINANCE_ENABLED)
#include "brave/components/binance/browser/regions.h"
#endif

#if BUILDFLAG(BRAVE_TOGETHER_ENABLED)
#include "brave/components/brave_together/browser/regions.h"
#endif

using ntp_background_images::ViewCounterServiceFactory;
using ntp_background_images::prefs::kNewTabPageSuperReferralThemesOption;

namespace {

bool IsSuperReferralActive(Profile* profile) {
  bool isSuperReferralActive = false;
  auto* service = ViewCounterServiceFactory::GetForProfile(profile);
  if (service) {
    auto* data = service->GetCurrentBrandedWallpaperData();
    if (data && data->IsSuperReferral()) {
      isSuperReferralActive = true;
    }
  }

  return isSuperReferralActive;
}

}  // namespace

BraveAppearanceHandler::BraveAppearanceHandler() {
  local_state_change_registrar_.Init(g_browser_process->local_state());
  local_state_change_registrar_.Add(
      kBraveDarkMode,
      base::Bind(&BraveAppearanceHandler::OnBraveDarkModeChanged,
                 base::Unretained(this)));
}

BraveAppearanceHandler::~BraveAppearanceHandler() = default;

void BraveAppearanceHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  profile_state_change_registrar_.Init(profile_->GetPrefs());
  profile_state_change_registrar_.Add(
      kNewTabPageSuperReferralThemesOption,
      base::BindRepeating(&BraveAppearanceHandler::OnPreferenceChanged,
      base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "setBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::SetBraveThemeType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::GetBraveThemeType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getIsSuperReferralActive",
      base::BindRepeating(&BraveAppearanceHandler::GetIsSuperReferralActive,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getIsBinanceSupported",
      base::BindRepeating(&BraveAppearanceHandler::GetIsBinanceSupported,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getIsBraveTogetherSupported",
      base::BindRepeating(&BraveAppearanceHandler::GetIsBraveTogetherSupported,
                          base::Unretained(this)));
}

void BraveAppearanceHandler::SetBraveThemeType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  AllowJavascript();

  int int_type;
  args->GetInteger(0, &int_type);
  dark_mode::SetBraveDarkModeType(
      static_cast<dark_mode::BraveDarkModeType>(int_type));
}

void BraveAppearanceHandler::GetBraveThemeType(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();
  // GetBraveThemeType() should be used because settings option displays all
  // available options including default.
  ResolveJavascriptCallback(
      args->GetList()[0],
      base::Value(static_cast<int>(dark_mode::GetBraveDarkModeType())));
}

void BraveAppearanceHandler::GetIsSuperReferralActive(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();
  ResolveJavascriptCallback(args->GetList()[0],
                            base::Value(IsSuperReferralActive(profile_)));
}

void BraveAppearanceHandler::GetIsBinanceSupported(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();

#if !BUILDFLAG(BINANCE_ENABLED)
  bool is_supported = false;
#else
  bool is_supported = crypto_exchange::IsRegionSupported(
      profile_->GetPrefs(), binance::unsupported_regions, false);
#endif

  ResolveJavascriptCallback(args->GetList()[0], base::Value(is_supported));
}

void BraveAppearanceHandler::GetIsBraveTogetherSupported(
    const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);

  AllowJavascript();

#if !BUILDFLAG(BRAVE_TOGETHER_ENABLED)
  bool is_supported = false;
#else
  bool is_supported = crypto_exchange::IsRegionSupported(
      profile_->GetPrefs(), brave_together::supported_regions, true);
#endif

  ResolveJavascriptCallback(args->GetList()[0], base::Value(is_supported));
}

void BraveAppearanceHandler::OnBraveDarkModeChanged() {
  // GetBraveThemeType() should be used because settings option displays all
  // available options including default.
  if (IsJavascriptAllowed()) {
    FireWebUIListener(
        "brave-theme-type-changed",
        base::Value(static_cast<int>(dark_mode::GetBraveDarkModeType())));
  }
}

void BraveAppearanceHandler::OnPreferenceChanged(const std::string& pref_name) {
  DCHECK_EQ(kNewTabPageSuperReferralThemesOption, pref_name);
  if (IsJavascriptAllowed()) {
    FireWebUIListener("super-referral-active-state-changed",
                      base::Value(IsSuperReferralActive(profile_)));
  }
}
