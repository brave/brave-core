/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 #include "brave/browser/ui/views/autofill/confirm_autocomplete_bubble_view.h"
 #include "chrome/browser/ui/views/autofill/autofill_bubble_handler_impl.h"

#define ShowMandatoryReauthBubble                                                 \
  ShowConfirmAutocompleteBubble(                                                  \
      content::WebContents* web_contents,                                         \
      ConfirmAutocompleteBubbleController* controller) {                          \
    views::View* anchor_view = toolbar_button_provider_->GetAnchorView(           \
        PageActionIconType::kSaveAutofillAddress);                                \
    ConfirmAutocompleteBubbleView* bubble =                                       \
        new ConfirmAutocompleteBubbleView(anchor_view, web_contents, controller); \
                                                                                  \
    PageActionIconView* icon_view =                                               \
        toolbar_button_provider_->GetPageActionIconView(                          \
            PageActionIconType::kSaveAutofillAddress);                            \
    DCHECK(icon_view);                                                            \
    bubble->SetHighlightedButton(icon_view);                                      \
                                                                                  \
    views::BubbleDialogDelegateView::CreateBubble(bubble);                        \
    bubble->Show(LocationBarBubbleDelegateView::AUTOMATIC);                       \
    return bubble;                                                                \
  }                                                                               \
                                                                                  \
  AutofillBubbleBase* AutofillBubbleHandlerImpl::ShowMandatoryReauthBubble

#include "src/chrome/browser/ui/views/autofill/autofill_bubble_handler_impl.cc"
#undef ShowMandatoryReauthBubble
