/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "chrome/browser/chooser_controller/title_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#endif

namespace {

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
std::u16string BraveCreateTitleLabel() {
  auto wallet_title = l10n_util::GetStringUTF16(IDS_BRAVE_WALLET);
  return l10n_util::GetStringFUTF16(IDS_HID_CHOOSER_PROMPT, wallet_title);
}
#endif

}  // namespace

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#define CreateChooserTitle                                                  \
  brave_wallet::IsBraveWalletOrigin(                                        \
      render_frame_host->GetOutermostMainFrame()->GetLastCommittedOrigin()) \
      ? BraveCreateTitleLabel()                                             \
      : CreateChooserTitle
#endif
#include <chrome/browser/ui/hid/hid_chooser_controller.cc>

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#undef CreateChooserTitle
#endif
