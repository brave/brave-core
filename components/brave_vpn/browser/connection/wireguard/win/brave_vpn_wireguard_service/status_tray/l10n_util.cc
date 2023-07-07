/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/l10n_util.h"

#include <string>
#include <vector>

#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/atl.h"
#include "base/win/embedded_i18n/language_selector.h"
#include "base/win/i18n.h"
#include "brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/status_tray/resources/status_tray_strings.h"

namespace brave_vpn {
namespace {

constexpr base::win::i18n::LanguageSelector::LangToOffset
    kLanguageOffsetPairs[] = {
#define HANDLE_LANGUAGE(l_, o_) {L## #l_, o_},
        DO_LANGUAGES
#undef HANDLE_LANGUAGE
};

std::wstring GetPreferredLanguage() {
  std::vector<std::wstring> languages;
  if (!base::win::i18n::GetUserPreferredUILanguageList(&languages) ||
      languages.size() == 0) {
    return L"en-us";
  }

  return languages[0];
}

const base::win::i18n::LanguageSelector& GetLanguageSelector() {
  static base::NoDestructor<base::win::i18n::LanguageSelector> instance(
      GetPreferredLanguage(), kLanguageOffsetPairs);
  return *instance;
}

}  // namespace

std::u16string GetLocalizedString(UINT base_message_id) {
  UINT message_id =
      static_cast<UINT>(base_message_id + GetLanguageSelector().offset());
  const ATLSTRINGRESOURCEIMAGE* image =
      AtlGetStringResourceImage(_AtlBaseModule.GetModuleInstance(), message_id);
  if (image) {
    return base::WideToUTF16(std::wstring(image->achString, image->nLength));
  }
  NOTREACHED() << "Unable to find resource id " << message_id;
  return std::u16string();
}

}  // namespace brave_vpn
