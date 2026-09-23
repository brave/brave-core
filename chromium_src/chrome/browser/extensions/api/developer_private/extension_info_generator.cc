/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/extensions/api/developer_private.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/manifest_handlers/incognito_info.h"

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

#define BRAVE_CREATE_EXTENSION_INFO_HELPER \
  info.is_split_mode = IncognitoInfo::IsSplitMode(&extension);
#include <chrome/browser/extensions/api/developer_private/extension_info_generator.cc>
#undef BRAVE_CREATE_EXTENSION_INFO_HELPER
