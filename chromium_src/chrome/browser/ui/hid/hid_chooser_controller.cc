/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/wallet_common_ui.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/chooser_controller/title_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

std::u16string BraveCreateTitleLabel() {
  auto wallet_title =
      brave_l10n::GetLocalizedResourceUTF16String(IDS_BRAVE_WALLET);
  return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_PROMPT, wallet_title);
}

}  // namespace

#define CreateChooserTitle                                                  \
  brave_wallet::IsBraveWalletOrigin(                                        \
      render_frame_host->GetOutermostMainFrame()->GetLastCommittedOrigin()) \
      ? BraveCreateTitleLabel()                                             \
      : CreateChooserTitle
#include "src/chrome/browser/ui/hid/hid_chooser_controller.cc"
#undef CreateChooserTitle
