#ifndef BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_IMPL_H_
#define BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_IMPL_H_

#include "brave/browser/ui/autofill/confirm_autocomplete_bubble_controller.h"
#include "chrome/browser/ui/autofill/autofill_bubble_controller_base.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "content/public/browser/web_contents_user_data.h"

namespace autofill {

class ConfirmAutocompleteBubbleControllerImpl
    : public AutofillBubbleControllerBase,
      public ConfirmAutocompleteBubbleController,
      public content::WebContentsUserData<
          ConfirmAutocompleteBubbleControllerImpl> {
 public:
  ConfirmAutocompleteBubbleControllerImpl(
      const ConfirmAutocompleteBubbleControllerImpl&) = delete;
  ConfirmAutocompleteBubbleControllerImpl& operator=(
      const ConfirmAutocompleteBubbleControllerImpl&) = delete;

  ~ConfirmAutocompleteBubbleControllerImpl() override;

  void ShowBubble(AutofillClient::ConfirmAutocompletePromptCallback
                      confirm_autocomplete_prompt_callback);

  std::u16string GetWindowTitle() const override;
  std::u16string GetAcceptButtonText() const override;
  std::u16string GetDeclineButtonText() const override;

  void OnAcceptButton() override;
  void OnBubbleClosed(PaymentsBubbleClosedReason closed_reason) override;

 protected:
  explicit ConfirmAutocompleteBubbleControllerImpl(
      content::WebContents* web_contents);

  // AutofillBubbleControllerBase:
  PageActionIconType GetPageActionIconType() override;
  void DoShowBubble() override;

 private:
  friend class content::WebContentsUserData<
      ConfirmAutocompleteBubbleControllerImpl>;

  AutofillClient::ConfirmAutocompletePromptCallback
      local_confirm_autocomplete_prompt_callback_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_IMPL_H_
