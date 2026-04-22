// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/web_view/autofill/brave_autofill_controller.h"

#import <Foundation/Foundation.h>

#include <string>

#include "base/functional/callback_forward.h"
#include "base/notimplemented.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/web_view/autofill/brave_web_view_autofill_client.h"
#include "components/autofill/core/browser/foundations/autofill_client.h"
#include "components/autofill/core/browser/ui/payments/card_unmask_prompt_options.h"
#include "components/autofill/ios/browser/autofill_agent.h"
#include "components/autofill/ios/browser/autofill_client_ios_bridge.h"
#include "components/autofill/ios/browser/autofill_driver_ios_bridge.h"
#include "ios/web_view/internal/autofill/cwv_autofill_client_ios_bridge.h"
#include "ios/web_view/internal/autofill/cwv_autofill_controller+testing.h"
#include "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"
#include "ios/web_view/internal/autofill/cwv_autofill_form_internal.h"
#include "ios/web_view/internal/autofill/cwv_autofill_profile_internal.h"
#include "ios/web_view/public/cwv_autofill_controller_delegate.h"
#include "ios/web_view/public/cwv_autofill_form.h"
#include "ios/web_view/public/cwv_autofill_profile.h"
#include "ios/web_view/public/cwv_autofill_suggestion.h"

using UserDecision = autofill::AutofillClient::AddressPromptUserDecision;

@interface BraveAutofillClientBridge
    : NSObject <CWVAutofillClientIOSBridge, AutofillDriverIOSBridge>
@property(nonatomic, weak) AutofillAgent* autofillAgent;
@property(nonatomic, weak) BraveAutofillController* autofillController;
- (instancetype)initWithAutofillAgent:(AutofillAgent*)autofillAgent;
@end

@interface BraveAutofillController ()
@property(nonatomic) BraveAutofillClientBridge* bridge;
@end

@implementation BraveAutofillController

- (instancetype)initWithWebState:(web::WebState*)webState
                   autofillAgent:(AutofillAgent*)autofillAgent
                 passwordManager:
                     (std::unique_ptr<password_manager::PasswordManager>)
                         passwordManager
           passwordManagerClient:
               (std::unique_ptr<ios_web_view::WebViewPasswordManagerClient>)
                   passwordManagerClient
              passwordController:(SharedPasswordController*)passwordController {
  BraveAutofillClientBridge* bridge =
      [[BraveAutofillClientBridge alloc] initWithAutofillAgent:autofillAgent];
  self =
      [super initWithWebState:webState
          autofillClientForTest:autofill::BraveWebViewAutofillClientIOS::Create(
                                    webState, bridge)
                  autofillAgent:autofillAgent
                passwordManager:std::move(passwordManager)
          passwordManagerClient:std::move(passwordManagerClient)
             passwordController:passwordController];
  self.bridge = bridge;
  bridge.autofillController = self;
  return self;
}

@end

@implementation BraveAutofillClientBridge

- (instancetype)initWithAutofillAgent:(AutofillAgent*)autofillAgent {
  if ((self = [super init])) {
    _autofillAgent = autofillAgent;
  }
  return self;
}

#pragma mark - AutofillClientIOSBridge

- (void)showAutofillPopup:(const std::vector<autofill::Suggestion>&)suggestions
       suggestionDelegate:
           (const base::WeakPtr<autofill::AutofillSuggestionDelegate>&)
               delegate {
  // We only want Autofill suggestions.
  std::vector<autofill::Suggestion> filtered_suggestions;
  std::ranges::copy_if(
      suggestions, std::back_inserter(filtered_suggestions),
      [](const autofill::Suggestion& suggestion) {
        return suggestion.type == autofill::SuggestionType::kAddressEntry ||
               suggestion.type == autofill::SuggestionType::kCreditCardEntry;
      });
  [_autofillAgent showAutofillPopup:filtered_suggestions
                 suggestionDelegate:delegate];
}

- (void)hideAutofillPopup {
  [_autofillAgent hideAutofillPopup];
}

- (bool)isLastQueriedField:(autofill::FieldGlobalId)fieldId {
  return [_autofillAgent isLastQueriedField:fieldId];
}

- (void)showPlusAddressEmailOverrideNotification:
    (base::OnceClosure)emailOverrideUndoCallback {
  NOTIMPLEMENTED();
}

#pragma mark - AutofillDriverIOSBridge

- (void)fillData:(const std::vector<autofill::FormFieldData::FillData>&)fields
           section:(const autofill::Section&)section
           inFrame:(web::WebFrame*)frame
    withActionType:(autofill::mojom::FormActionType)actionType {
  [_autofillAgent fillData:fields
                   section:section
                   inFrame:frame
            withActionType:actionType];
}

- (void)fillSpecificFormField:(const autofill::FieldRendererId&)field
                    withValue:(const std::u16string)value
                      inFrame:(web::WebFrame*)frame {
  NOTIMPLEMENTED();
}

- (void)handleParsedForms:
            (const std::vector<raw_ref<const autofill::FormStructure>>&)forms
                  inFrame:(web::WebFrame*)frame {
  if (![self.autofillController.delegate
          respondsToSelector:@selector(autofillController:
                                             didFindForms:frameID:)]) {
    return;
  }

  NSMutableArray<CWVAutofillForm*>* autofillForms = [NSMutableArray array];
  for (const raw_ref<const autofill::FormStructure>& form : forms) {
    CWVAutofillForm* autofillForm =
        [[CWVAutofillForm alloc] initWithFormStructure:*form];
    [autofillForms addObject:autofillForm];
  }
  [self.autofillController.delegate
      autofillController:self.autofillController
            didFindForms:autofillForms
                 frameID:base::SysUTF8ToNSString(frame->GetFrameId())];
}

- (void)fillFormDataPredictions:
            (const std::vector<autofill::FormDataPredictions>&)forms
                        inFrame:(web::WebFrame*)frame {
  NOTIMPLEMENTED();
}

- (void)fetchFormsFiltered:(std::optional<std::u16string>)formNameFilter
                   inFrame:(web::WebFrame*)frame
         completionHandler:(FormFetchCompletion)completionHandler {
  [_autofillAgent fetchFormsFiltered:std::move(formNameFilter)
                             inFrame:frame
                   completionHandler:std::move(completionHandler)];
}

- (void)notifyFormsSeen:(const std::vector<autofill::FormData>&)updatedForms
                inFrame:(web::WebFrame*)frame {
  [_autofillAgent notifyFormsSeen:updatedForms inFrame:frame];
}

#pragma mark - CWVAutofillClientIOSBridge

- (void)
    showSaveCreditCardToCloud:(const autofill::CreditCard&)creditCard
            legalMessageLines:(autofill::LegalMessageLines)legalMessageLines
        saveCreditCardOptions:
            (autofill::payments::PaymentsAutofillClient::SaveCreditCardOptions)
                saveCreditCardOptions
                     callback:(autofill::payments::PaymentsAutofillClient::
                                   UploadSaveCardPromptCallback)callback {
  NOTIMPLEMENTED();
}

- (void)handleCreditCardUploadCompleted:(BOOL)cardSaved
                               callback:(base::OnceClosure)callback {
  NOTIMPLEMENTED();
}

- (void)showUnmaskPromptForCard:(const autofill::CreditCard&)creditCard
        cardUnmaskPromptOptions:
            (const autofill::CardUnmaskPromptOptions&)cardUnmaskPromptOptions
                       delegate:(base::WeakPtr<autofill::CardUnmaskDelegate>)
                                    delegate {
  NOTIMPLEMENTED();
}

- (void)didReceiveUnmaskVerificationResult:
    (autofill::payments::PaymentsAutofillClient::PaymentsRpcResult)result {
  NOTIMPLEMENTED();
}

- (void)loadRiskData:(base::OnceCallback<void(const std::string&)>)callback {
  NOTIMPLEMENTED();
}

- (void)
    confirmSaveAddressProfile:(const autofill::AutofillProfile&)profile
              originalProfile:(const autofill::AutofillProfile*)originalProfile
                     callback:(autofill::AutofillClient::
                                   AddressProfileSavePromptCallback)callback {
  if ([self.autofillController.delegate
          respondsToSelector:@selector
          (autofillController:
              confirmSaveForNewAutofillProfile:oldProfile:decisionHandler:)]) {
    CWVAutofillProfile* newProfile =
        [[CWVAutofillProfile alloc] initWithProfile:profile];
    CWVAutofillProfile* oldProfile = nil;
    if (originalProfile) {
      oldProfile =
          [[CWVAutofillProfile alloc] initWithProfile:*originalProfile];
    }
    __block auto scopedCallback = std::move(callback);
    [self.autofillController.delegate
                      autofillController:self.autofillController
        confirmSaveForNewAutofillProfile:newProfile
                              oldProfile:oldProfile
                         decisionHandler:^(
                             CWVAutofillProfileUserDecision decision) {
                           UserDecision userDecision;
                           switch (decision) {
                             case CWVAutofillProfileUserDecisionAccepted:
                               userDecision = UserDecision::kAccepted;
                               break;
                             case CWVAutofillProfileUserDecisionDeclined:
                               userDecision = UserDecision::kDeclined;
                               break;
                             case CWVAutofillProfileUserDecisionIgnored:
                               userDecision = UserDecision::kIgnored;
                               break;
                           }
                           std::move(scopedCallback)
                               .Run(userDecision, *newProfile.internalProfile);
                         }];
  } else {
    std::move(callback).Run(UserDecision::kUserNotAsked, profile);
  }
}

- (void)showAutofillProgressDialogOfType:(autofill::AutofillProgressUiType)type
                          cancelCallback:(base::OnceClosure)cancelCallback {
  NOTIMPLEMENTED();
}

- (void)closeAutofillProgressDialogWithConfirmation:(BOOL)showConfirmation
                                 completionCallback:
                                     (base::OnceClosure)callback {
  NOTIMPLEMENTED();
}

- (void)showUnmaskAuthenticatorSelectorWithOptions:
            (const std::vector<autofill::CardUnmaskChallengeOption>&)options
                                    acceptCallback:
                                        (base::OnceCallback<void(
                                             const std::string&)>)acceptCallback
                                    cancelCallback:
                                        (base::OnceClosure)cancelCallback {
  NOTIMPLEMENTED();
}

- (void)showVirtualCardEnrollmentWithEnrollmentFields:
            (const autofill::VirtualCardEnrollmentFields&)enrollmentFields
                                       acceptCallback:
                                           (base::OnceClosure)acceptCallback
                                      declineCallback:
                                          (base::OnceClosure)declineCallback {
  NOTIMPLEMENTED();
}

- (void)handleVirtualCardEnrollmentResult:(BOOL)cardEnrolled {
  NOTIMPLEMENTED();
}

- (void)showCardUnmaskOtpInputDialogForCardType:
            (autofill::CreditCard::RecordType)cardType
                                challengeOption:
                                    (const autofill::CardUnmaskChallengeOption&)
                                        challengeOption
                                       delegate:
                                           (base::WeakPtr<
                                               autofill::OtpUnmaskDelegate>)
                                               delegate {
  NOTIMPLEMENTED();
}

- (void)didReceiveUnmaskOtpVerificationResult:
    (autofill::OtpUnmaskResult)unmaskResult {
  NOTIMPLEMENTED();
}

@end
