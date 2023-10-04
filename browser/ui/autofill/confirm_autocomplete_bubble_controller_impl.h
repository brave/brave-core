// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_IMPL_H_
#define BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_IMPL_H_

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/autofill/confirm_autocomplete_bubble_controller.h"
#include "brave/browser/ui/autofill/confirm_autocomplete_ui.h"
#include "chrome/browser/ui/autofill/autofill_bubble_controller_base.h"
#include "chrome/browser/ui/autofill/payments/save_payment_icon_controller.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "content/public/browser/web_contents_user_data.h"

namespace autofill {

class ConfirmAutocompleteBubbleControllerImpl
    : public AutofillBubbleControllerBase,
      public ConfirmAutocompleteBubbleController,
      public SavePaymentIconController,
      public content::WebContentsUserData<
          ConfirmAutocompleteBubbleControllerImpl> {
 public:
  ConfirmAutocompleteBubbleControllerImpl(
      const ConfirmAutocompleteBubbleControllerImpl&) = delete;
  ConfirmAutocompleteBubbleControllerImpl& operator=(
      const ConfirmAutocompleteBubbleControllerImpl&) = delete;

  ~ConfirmAutocompleteBubbleControllerImpl() override;

  void OfferLocalSave(AutofillClient::ConfirmAutocompletePromptCallback
                          confirm_autocomplete_prompt_callback);

  // No-op if the bubble is already shown, otherwise, shows the bubble.
  void ReshowBubble();

  std::u16string GetWindowTitle() const override;
  std::u16string GetAcceptButtonText() const override;
  std::u16string GetDeclineButtonText() const override;

  void OnAcceptButton() override;
  void OnBubbleClosed(PaymentsBubbleClosedReason closed_reason) override;
  ConfirmAutocompleteBubbleType GetBubbleType() const override;

  // SavePaymentIconController:
  std::u16string GetSavePaymentIconTooltipText() const override;
  bool ShouldShowSavingPaymentAnimation() const override;
  bool ShouldShowPaymentSavedLabelAnimation() const override;
  bool ShouldShowSaveFailureBadge() const override;
  void OnAnimationEnded() override;
  bool IsIconVisible() const override;
  AutofillBubbleBase* GetPaymentBubbleView() const override;
  PaymentBubbleType GetPaymentBubbleType() const override;
  int GetSaveSuccessAnimationStringId() const override;

 protected:
  explicit ConfirmAutocompleteBubbleControllerImpl(
      content::WebContents* web_contents);

  // AutofillBubbleControllerBase:
  PageActionIconType GetPageActionIconType() override;
  void DoShowBubble() override;

 private:
  friend class content::WebContentsUserData<
      ConfirmAutocompleteBubbleControllerImpl>;

  // Should outlive this object.
  raw_ptr<PersonalDataManager> personal_data_manager_;

  // Note: Below fields are set when IBAN save is offered.
  //
  // Is true only if the [IBAN saved] label animation should be shown.
  bool should_show_iban_saved_label_animation_ = false;

  ConfirmAutocompleteBubbleType current_bubble_type_ =
      ConfirmAutocompleteBubbleType::kInactive;

  // Callback to run once the user makes a decision with respect to the local
  // IBAN offer-to-save prompt.
  AutofillClient::ConfirmAutocompletePromptCallback
      local_confirm_autocomplete_prompt_callback_;

  // Whether the bubble is shown after user interacted with the omnibox icon.
  bool is_reshow_ = false;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_IMPL_H_
