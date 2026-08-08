/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/extension_malware_blocklist/browser/extension_malware_blocklist.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/common/extensions/api/developer_private.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void ProcessKnownMV2Extensions(
    extensions::api::developer_private::ExtensionInfo& info) {
  if (!extensions_mv2::features::IsExtensionReplacementEnabled()) {
    return;
  }
  if (extensions_mv2::IsKnownWebStoreHostedExtension(info.id)) {
    // Suppress mv2 messages for known extensions (which are being replaced with
    // brave-hosted versions) on brave://extensions
    info.is_affected_by_mv2_deprecation = false;
    info.disable_reasons.unsupported_manifest_version = false;
  }
}

}  // namespace

#define BRAVE_CREATE_EXTENSION_INFO_HELPER                                  \
  info.is_split_mode = IncognitoInfo::IsSplitMode(&extension);              \
  if (auto* brave_malware_blocklist = extension_malware_blocklist::         \
          ExtensionMalwareBlocklist::GetInstance();                         \
      info.blocklist_text.has_value() && brave_malware_blocklist &&         \
      brave_malware_blocklist->IsMalware(extension.id())) {                 \
    info.blocklist_text =                                                   \
        l10n_util::GetStringUTF8(IDS_BRAVE_EXTENSIONS_BLOCKLISTED_MALWARE); \
  }
#include <chrome/browser/extensions/api/developer_private/extension_info_generator.cc>
#undef BRAVE_CREATE_EXTENSION_INFO_HELPER
