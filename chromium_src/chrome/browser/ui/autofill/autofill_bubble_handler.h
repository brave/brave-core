/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_BUBBLE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_AUTOFILL_BUBBLE_HANDLER_H_

namespace autofill {
class ConfirmAutocompleteBubbleController;
}

#define ShowMandatoryReauthBubble                           \
  ShowConfirmAutocompleteBubble(                            \
      content::WebContents* web_contents,                   \
      ConfirmAutocompleteBubbleController* controller) = 0; \
  virtual AutofillBubbleBase* ShowMandatoryReauthBubble

#include "src/chrome/browser/ui/autofill/autofill_bubble_handler.h"

#undef ShowMandatoryReauthBubble

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_SYNC_CLIENT_H_
