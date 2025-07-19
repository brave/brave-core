/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/extensions/mv2_disabled_dialog_controller.h"

#include <algorithm>
#include <vector>

#include "brave/browser/extensions/manifest_v2/brave_hosted_extensions.h"
#include "brave/browser/extensions/manifest_v2/features.h"
#include "chrome/browser/ui/browser.h"

namespace {

bool MaybeEraseKnownMV2Extensions(
    std::vector<extensions::Mv2DisabledDialogController::ExtensionInfo>&
        extensions) {
  if (!extensions_mv2::features::IsExtensionReplacementEnabled()) {
    return false;
  }

  extensions.erase(
      std::remove_if(extensions.begin(), extensions.end(),
                     [](const auto& e) {
                       return extensions_mv2::IsKnownCwsMV2Extension(e.id);
                     }),
      extensions.end());

  return false;
}

}  // namespace

#define window(...) \
  window(__VA_ARGS__) || MaybeEraseKnownMV2Extensions(affected_extensions_info_)

#include "src/chrome/browser/ui/extensions/mv2_disabled_dialog_controller.cc"

#undef window
