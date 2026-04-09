/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WALLET_WEBUI_CONTENTS_WRAPPER_H_
#define BRAVE_BROWSER_UI_WALLET_WEBUI_CONTENTS_WRAPPER_H_

#include "brave/browser/ui/webui/brave_wallet/wallet_panel_ui.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "components/input/native_web_keyboard_event.h"
#include "content/public/browser/keyboard_event_processing_result.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_wallet {

// Wallet-specific WebUIContentsWrapper that blocks keyboard events from
// reaching the WebContents when they should be treated as unintended (aligned
// with PermissionPromptBaseView) or when occluded by PiP.
class WalletWebUIContentsWrapper final
    : public WebUIContentsWrapperT<WalletPanelUI> {
 public:
  using WebUIContentsWrapperT<WalletPanelUI>::WebUIContentsWrapperT;

  // WebUIContentsWrapperT:
  content::KeyboardEventProcessingResult PreHandleKeyboardEvent(
      content::WebContents* source,
      const input::NativeWebKeyboardEvent& event) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WALLET_WEBUI_CONTENTS_WRAPPER_H_
