/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/autofill/confirm_autocomplete_bubble_controller_impl.h"

#include <string>

#include "absl/types/optional.h"
#include "chrome/browser/ui/autofill/autofill_bubble_handler.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill {

ConfirmAutocompleteBubbleControllerImpl::
    ~ConfirmAutocompleteBubbleControllerImpl() = default;

void ConfirmAutocompleteBubbleControllerImpl::ShowBubble(
    base::OnceCallback<void(absl::optional<bool>)> callback) {
  if (bubble_view()) {
    return;
  }

  callback_ = std::move(callback);
  DCHECK(callback_);

  Show();
}

std::u16string ConfirmAutocompleteBubbleControllerImpl::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(
      IDS_BRAVE_AUTOFILL_CONFIRM_AUTOCOMPLETE_PROMPT_TITLE);
}

std::u16string ConfirmAutocompleteBubbleControllerImpl::GetAcceptButtonText()
    const {
  return l10n_util::GetStringUTF16(IDS_OK);
}

std::u16string ConfirmAutocompleteBubbleControllerImpl::GetDeclineButtonText()
    const {
  return l10n_util::GetStringUTF16(IDS_NO_THANKS);
}

void ConfirmAutocompleteBubbleControllerImpl::OnAcceptButton() {
  DCHECK(callback_);
  std::move(callback_).Run(true);
}

void ConfirmAutocompleteBubbleControllerImpl::OnBubbleClosed(
    views::Widget::ClosedReason closed_reason) {
  if (closed_reason != views::Widget::ClosedReason::kAcceptButtonClicked) {
    DCHECK(callback_);
    std::move(callback_).Run(
        closed_reason == views::Widget::ClosedReason::kCancelButtonClicked
            ? absl::make_optional(false)
            : absl::nullopt);
  }

  set_bubble_view(nullptr);

  UpdatePageActionIcon();
}

ConfirmAutocompleteBubbleControllerImpl::
    ConfirmAutocompleteBubbleControllerImpl(content::WebContents* web_contents)
    : AutofillBubbleControllerBase(web_contents),
      content::WebContentsUserData<ConfirmAutocompleteBubbleControllerImpl>(
          *web_contents) {}

PageActionIconType
ConfirmAutocompleteBubbleControllerImpl::GetPageActionIconType() {
  return PageActionIconType::kSaveIban;
}

void ConfirmAutocompleteBubbleControllerImpl::DoShowBubble() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  AutofillBubbleHandler* autofill_bubble_handler =
      browser->window()->GetAutofillBubbleHandler();
  set_bubble_view(autofill_bubble_handler->ShowConfirmAutocompleteBubble(
      web_contents(), this));
  DCHECK(bubble_view());
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ConfirmAutocompleteBubbleControllerImpl);

}  // namespace autofill
