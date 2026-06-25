/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/extensions/mv2_disabled_dialog_controller.h"

#include <algorithm>
#include <vector>

#include "chrome/browser/ui/browser.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_features.h"

namespace {

void MaybeEraseKnownMV2Extensions(
    std::vector<extensions::Mv2DisabledDialogController::ExtensionInfo>&
        extensions) {
  if (!extensions_mv2::features::IsExtensionReplacementEnabled()) {
    return;
  }

  std::erase_if(extensions, [](const auto& e) {
    return extensions_mv2::IsKnownWebStoreHostedExtension(e.id);
  });
}

}  // namespace

#include <chrome/browser/ui/extensions/mv2_disabled_dialog_controller.cc>
