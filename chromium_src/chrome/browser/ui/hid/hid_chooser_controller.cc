/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "chrome/browser/chooser_controller/title_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

std::u16string BraveCreateTitleLabel() {
  auto wallet_title = l10n_util::GetStringUTF16(IDS_BRAVE_WALLET);
  return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_PROMPT_ORIGIN,
                                    wallet_title);
}

}  // namespace

#define CreateExtensionAwareChooserTitle           \
  brave_wallet::IsBraveWalletOrigin(               \
      render_frame_host->GetLastCommittedOrigin()) \
      ? BraveCreateTitleLabel()                    \
      : CreateExtensionAwareChooserTitle
#include "src/chrome/browser/ui/hid/hid_chooser_controller.cc"
#undef CreateExtensionAwareChooserTitle
