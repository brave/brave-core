/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/device_chooser_content_view.h"

#include "brave/browser/ui/views/dialog_footnote_utils.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

#include "src/chrome/browser/ui/views/device_chooser_content_view.cc"

std::unique_ptr<views::View> DeviceChooserContentView::CreateFootnoteView(
    Browser* browser) {
  if (chooser_controller_->GetType() ==
      permissions::ChooserControllerType::kBluetooth) {
    return views::CreateStyledLabelForDialogFootnote(
        browser,
        l10n_util::GetStringUTF16(
            IDS_PERMISSIONS_BLUETOOTH_CHOOSER_PRIVACY_WARNING_TEXT),
        {brave_l10n::GetLocalizedResourceUTF16String(IDS_LEARN_MORE)},
        {GURL(kPermissionPromptHardwareAccessPrivacyRisksURL)});
  }

  return nullptr;
}
