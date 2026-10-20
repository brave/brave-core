// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/toolbar/brave_transfer_button.h"

#include "base/functional/bind.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "url/gurl.h"

BraveTransferButton::BraveTransferButton(
    BrowserWindowInterface* browser_window_interface)
    : ToolbarButton(base::BindRepeating(&BraveTransferButton::ButtonPressed,
                                        base::Unretained(this))),
      browser_window_interface_(browser_window_interface) {
  SetVectorIcon(kLeoSmartphoneLaptopIcon);
  SetTooltipText(
      l10n_util::GetStringUTF16(IDS_BRAVE_TRANSFER_TOOLBAR_BUTTON_TOOLTIP));
}

BraveTransferButton::~BraveTransferButton() {
  SetCallback(PressedCallback());
}

void BraveTransferButton::ButtonPressed() {
  ShowSingletonTab(browser_window_interface_, GURL(kBraveTransferURL));
}

BEGIN_METADATA(BraveTransferButton)
END_METADATA
