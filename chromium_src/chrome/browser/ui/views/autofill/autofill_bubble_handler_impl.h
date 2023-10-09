/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_BUBBLE_HANDLER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_BUBBLE_HANDLER_IMPL_H_

#include "chrome/browser/ui/autofill/autofill_bubble_handler.h"

namespace autofill {
  class ConfirmAutocompleteBubbleController;
}

#define ShowMandatoryReauthBubble                                \
  ShowConfirmAutocompleteBubble(                                 \
      content::WebContents* web_contents,                        \
      ConfirmAutocompleteBubbleController* controller) override; \
  AutofillBubbleBase* ShowMandatoryReauthBubble

#include "src/chrome/browser/ui/views/autofill/autofill_bubble_handler_impl.h"
#undef ShowMandatoryReauthBubble

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_AUTOFILL_AUTOFILL_BUBBLE_HANDLER_IMPL_H_
