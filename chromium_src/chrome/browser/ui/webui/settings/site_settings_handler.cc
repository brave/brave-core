/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/site_settings_handler.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"

namespace {
constexpr int kOriginalNotValidWebAddressId =
    IDS_SETTINGS_NOT_VALID_WEB_ADDRESS_FOR_CONTENT_TYPE;
}

#undef IDS_SETTINGS_NOT_VALID_WEB_ADDRESS_FOR_CONTENT_TYPE

#define IDS_SETTINGS_NOT_VALID_WEB_ADDRESS_FOR_CONTENT_TYPE                \
  kOriginalNotValidWebAddressId);                                          \
  return false;                                                            \
  }                                                                        \
  if (!brave_shields::IsPatternValidForBraveContentType(content_type,      \
                                                        pattern_string)) { \
    *out_error = l10n_util::GetStringUTF8( \
        IDS_BRAVE_SHIELDS_NOT_VALID_ADDRESS
#include "src/chrome/browser/ui/webui/settings/site_settings_handler.cc"
#undef IDS_SETTINGS_NOT_VALID_WEB_ADDRESS_FOR_CONTENT_TYPE
#define IDS_SETTINGS_NOT_VALID_WEB_ADDRESS_FOR_CONTENT_TYPE \
  kOriginalNotValidWebAddressId
