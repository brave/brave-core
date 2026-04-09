/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/wallet_webui_contents_wrapper.h"

#include "content/public/browser/keyboard_event_processing_result.h"
#include "content/public/browser/web_contents.h"

namespace brave_wallet {

content::KeyboardEventProcessingResult
WalletWebUIContentsWrapper::PreHandleKeyboardEvent(
    content::WebContents* source,
    const input::NativeWebKeyboardEvent& event) {
  WebUIContentsWrapper::Host* host = GetHost().get();
  if (host && host->PreHandleKeyboardEvent(source, event) ==
                  content::KeyboardEventProcessingResult::HANDLED) {
    return content::KeyboardEventProcessingResult::HANDLED;
  }

  return WebUIContentsWrapperT<WalletPanelUI>::PreHandleKeyboardEvent(source,
                                                                      event);
}

}  // namespace brave_wallet
