/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tabs/credit_card_field_focus_observer.h"

#include <utility>

#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/autofill_field.h"
#include "components/autofill/core/browser/form_structure.h"
#include "content/public/browser/web_contents.h"

namespace brave_ads {

namespace {

bool IsCreditCardField(autofill::AutofillManager& manager,
                       autofill::FieldGlobalId field) {
  const autofill::FormStructure* const form_structure =
      manager.FindCachedFormById(field);
  if (!form_structure) {
    return false;
  }

  const autofill::AutofillField* const autofill_field =
      form_structure->GetFieldById(field);
  return autofill_field && autofill_field->Type().GetGroups().contains(
                               autofill::FieldTypeGroup::kCreditCard);
}

}  // namespace

CreditCardFieldFocusObserver::CreditCardFieldFocusObserver(
    content::WebContents& web_contents,
    base::RepeatingClosure credit_card_field_focused_callback)
    : CreditCardFieldFocusObserver(
          web_contents,
          std::move(credit_card_field_focused_callback),
          base::BindRepeating(&IsCreditCardField)) {}

CreditCardFieldFocusObserver::CreditCardFieldFocusObserver(
    content::WebContents& web_contents,
    base::RepeatingClosure credit_card_field_focused_callback,
    CreditCardFieldFocusObserver::CreditCardFieldClassifier
        credit_card_field_classifier)
    : credit_card_field_focused_callback_(
          std::move(credit_card_field_focused_callback)),
      credit_card_field_classifier_(std::move(credit_card_field_classifier)) {
  autofill::ContentAutofillDriver* const autofill_driver =
      autofill::ContentAutofillDriver::GetForRenderFrameHost(
          web_contents.GetPrimaryMainFrame());
  if (!autofill_driver) {
    return;
  }

  autofill_manager_observation_.Observe(&autofill_driver->GetAutofillManager());
}

CreditCardFieldFocusObserver::~CreditCardFieldFocusObserver() = default;

void CreditCardFieldFocusObserver::OnAutofillManagerStateChanged(
    autofill::AutofillManager& /*manager*/,
    autofill::AutofillManager::LifecycleState /*previous_state*/,
    autofill::AutofillManager::LifecycleState new_state) {
  // The observed `AutofillManager` is tied to a `RenderFrameHost`, not to
  // this observer's own lifetime, so it can be torn down (e.g. on
  // navigation) well before `AdsTabHelper` replaces or resets this observer.
  // Reset the observation here rather than leaving `ScopedObservation` to
  // deregister from an already-destroyed `AutofillManager` later.
  if (new_state == autofill::AutofillManager::LifecycleState::kPendingReset ||
      new_state ==
          autofill::AutofillManager::LifecycleState::kPendingDeletion) {
    autofill_manager_observation_.Reset();
  }
}

void CreditCardFieldFocusObserver::OnAfterFocusOnFormField(
    autofill::AutofillManager& manager,
    autofill::FormGlobalId form,
    autofill::FieldGlobalId field) {
  if (credit_card_field_classifier_.Run(manager, field)) {
    credit_card_field_focused_callback_.Run();
  }
}

}  // namespace brave_ads
