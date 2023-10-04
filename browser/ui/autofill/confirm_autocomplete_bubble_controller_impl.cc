// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/autofill/confirm_autocomplete_bubble_controller_impl.h"

#include <string>

#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/autofill/autofill_bubble_base.h"
#include "chrome/browser/ui/autofill/autofill_bubble_handler.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/page_action/page_action_icon_type.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/autofill/core/browser/metrics/autofill_metrics.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace autofill {

// static
ConfirmAutocompleteBubbleController*
ConfirmAutocompleteBubbleController::GetOrCreate(
    content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }

  ConfirmAutocompleteBubbleControllerImpl::CreateForWebContents(web_contents);
  return ConfirmAutocompleteBubbleControllerImpl::FromWebContents(web_contents);
}

ConfirmAutocompleteBubbleControllerImpl::
    ~ConfirmAutocompleteBubbleControllerImpl() = default;

void ConfirmAutocompleteBubbleControllerImpl::OfferLocalSave(
    AutofillClient::ConfirmAutocompletePromptCallback
        confirm_autocomplete_prompt_callback) {
  // Don't show the bubble if it's already visible.
  if (bubble_view()) {
    return;
  }

  is_reshow_ = false;
  local_confirm_autocomplete_prompt_callback_ =
      std::move(confirm_autocomplete_prompt_callback);
  current_bubble_type_ = ConfirmAutocompleteBubbleType::kLocalSave;

  DCHECK(!local_confirm_autocomplete_prompt_callback_.is_null());
  Show();
}

void ConfirmAutocompleteBubbleControllerImpl::ReshowBubble() {
  // Don't show the bubble if it's already visible.
  if (bubble_view()) {
    return;
  }

  is_reshow_ = true;
  DCHECK(current_bubble_type_ != ConfirmAutocompleteBubbleType::kInactive);
  DCHECK(!local_confirm_autocomplete_prompt_callback_.is_null());
  Show();
}

std::u16string ConfirmAutocompleteBubbleControllerImpl::GetWindowTitle() const {
  switch (current_bubble_type_) {
    case ConfirmAutocompleteBubbleType::kLocalSave:
      return u"Would you like Brave to save this kind of information and fill "
             u"it in for you later?";
    case ConfirmAutocompleteBubbleType::kInactive:
      NOTREACHED();
      return u"";
  }
}

std::u16string ConfirmAutocompleteBubbleControllerImpl::GetAcceptButtonText()
    const {
  switch (current_bubble_type_) {
    case ConfirmAutocompleteBubbleType::kLocalSave:
      return u"Yes";
    case ConfirmAutocompleteBubbleType::kInactive:
      NOTREACHED();
      return u"";
  }
}

std::u16string ConfirmAutocompleteBubbleControllerImpl::GetDeclineButtonText()
    const {
  switch (current_bubble_type_) {
    case ConfirmAutocompleteBubbleType::kLocalSave:
      return u"No";
    case ConfirmAutocompleteBubbleType::kInactive:
      NOTREACHED();
      return u"";
  }
}

void ConfirmAutocompleteBubbleControllerImpl::OnAcceptButton() {
  switch (current_bubble_type_) {
    case ConfirmAutocompleteBubbleType::kLocalSave:
      DCHECK(!local_confirm_autocomplete_prompt_callback_.is_null());
      should_show_iban_saved_label_animation_ = true;
      std::move(local_confirm_autocomplete_prompt_callback_)
          .Run(AutofillClient::ConfirmAutocompleteUserDecision::kAccepted);
      return;
    case ConfirmAutocompleteBubbleType::kInactive:
      NOTREACHED();
  }
}

void ConfirmAutocompleteBubbleControllerImpl::OnBubbleClosed(
    PaymentsBubbleClosedReason closed_reason) {
  if (current_bubble_type_ == ConfirmAutocompleteBubbleType::kLocalSave) {
    if (closed_reason == PaymentsBubbleClosedReason::kCancelled) {
      std::move(local_confirm_autocomplete_prompt_callback_)
          .Run(AutofillClient::ConfirmAutocompleteUserDecision::kDeclined);
    } else if (closed_reason == PaymentsBubbleClosedReason::kClosed) {
      std::move(local_confirm_autocomplete_prompt_callback_)
          .Run(AutofillClient::ConfirmAutocompleteUserDecision::kIgnored);
    }
  }

  set_bubble_view(nullptr);

  // Handles `current_bubble_type_` change according to its current type and the
  // `closed_reason`.
  if (closed_reason == PaymentsBubbleClosedReason::kAccepted) {
    current_bubble_type_ = ConfirmAutocompleteBubbleType::kInactive;
  } else if (closed_reason == PaymentsBubbleClosedReason::kCancelled) {
    current_bubble_type_ = ConfirmAutocompleteBubbleType::kInactive;
  }
  UpdatePageActionIcon();
}

ConfirmAutocompleteBubbleControllerImpl::
    ConfirmAutocompleteBubbleControllerImpl(content::WebContents* web_contents)
    : AutofillBubbleControllerBase(web_contents),
      content::WebContentsUserData<ConfirmAutocompleteBubbleControllerImpl>(
          *web_contents),
      personal_data_manager_(
          PersonalDataManagerFactory::GetInstance()->GetForProfile(
              Profile::FromBrowserContext(web_contents->GetBrowserContext()))) {
}

ConfirmAutocompleteBubbleType
ConfirmAutocompleteBubbleControllerImpl::GetBubbleType() const {
  return current_bubble_type_;
}

std::u16string
ConfirmAutocompleteBubbleControllerImpl::GetSavePaymentIconTooltipText() const {
  switch (current_bubble_type_) {
    case ConfirmAutocompleteBubbleType::kLocalSave:
      return l10n_util::GetStringUTF16(IDS_TOOLTIP_SAVE_IBAN);
    case ConfirmAutocompleteBubbleType::kInactive:
      return std::u16string();
  }
}

bool ConfirmAutocompleteBubbleControllerImpl::ShouldShowSavingPaymentAnimation()
    const {
  return false;
}

bool ConfirmAutocompleteBubbleControllerImpl::
    ShouldShowPaymentSavedLabelAnimation() const {
  return should_show_iban_saved_label_animation_;
}

bool ConfirmAutocompleteBubbleControllerImpl::ShouldShowSaveFailureBadge()
    const {
  return false;
}

void ConfirmAutocompleteBubbleControllerImpl::OnAnimationEnded() {
  should_show_iban_saved_label_animation_ = false;
}

bool ConfirmAutocompleteBubbleControllerImpl::IsIconVisible() const {
  // If there is no bubble to show, then there should be no icon.
  return current_bubble_type_ != ConfirmAutocompleteBubbleType::kInactive;
}

AutofillBubbleBase*
ConfirmAutocompleteBubbleControllerImpl::GetPaymentBubbleView() const {
  return bubble_view();
}

SavePaymentIconController::PaymentBubbleType
ConfirmAutocompleteBubbleControllerImpl::GetPaymentBubbleType() const {
  switch (current_bubble_type_) {
    case ConfirmAutocompleteBubbleType::kLocalSave:
      return PaymentBubbleType::kSaveIban;
    case ConfirmAutocompleteBubbleType::kInactive:
      return PaymentBubbleType::kUnknown;
  }
}

int ConfirmAutocompleteBubbleControllerImpl::GetSaveSuccessAnimationStringId()
    const {
  return IDS_AUTOFILL_IBAN_SAVED;
}

PageActionIconType
ConfirmAutocompleteBubbleControllerImpl::GetPageActionIconType() {
  return PageActionIconType::kSaveIban;
}

void ConfirmAutocompleteBubbleControllerImpl::DoShowBubble() {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  AutofillBubbleHandler* autofill_bubble_handler =
      browser->window()->GetAutofillBubbleHandler();
  set_bubble_view(autofill_bubble_handler->ShowConfirmAutocompleteBubble(
      web_contents(), this, /*is_user_gesture=*/is_reshow_,
      current_bubble_type_));
  DCHECK(bubble_view());
  DCHECK(current_bubble_type_ != ConfirmAutocompleteBubbleType::kInactive);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ConfirmAutocompleteBubbleControllerImpl);

}  // namespace autofill
