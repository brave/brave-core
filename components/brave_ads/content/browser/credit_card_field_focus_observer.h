/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREDIT_CARD_FIELD_FOCUS_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREDIT_CARD_FIELD_FOCUS_OBSERVER_H_

#include "base/functional/callback.h"
#include "base/scoped_observation.h"
#include "components/autofill/core/browser/foundations/autofill_manager.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads {

// A checkout page's order value is often not rendered until the user focuses
// a payment field, so this triggers re-extraction at that point.
class CreditCardFieldFocusObserver final
    : public autofill::AutofillManager::Observer {
 public:
  // Classifies a focused field as a credit card field. Injectable so tests
  // can verify the observer-to-callback wiring without a real Autofill parse.
  using CreditCardFieldClassifier =
      base::RepeatingCallback<bool(autofill::AutofillManager&,
                                   autofill::FieldGlobalId)>;

  // Observes the primary main frame's `AutofillManager`; inert if no
  // `ContentAutofillDriver` exists yet, and stops silently on manager reset.
  CreditCardFieldFocusObserver(
      content::WebContents& web_contents,
      base::RepeatingClosure credit_card_field_focused_callback);
  CreditCardFieldFocusObserver(
      content::WebContents& web_contents,
      base::RepeatingClosure credit_card_field_focused_callback,
      CreditCardFieldClassifier credit_card_field_classifier);

  CreditCardFieldFocusObserver(const CreditCardFieldFocusObserver&) = delete;
  CreditCardFieldFocusObserver& operator=(const CreditCardFieldFocusObserver&) =
      delete;

  ~CreditCardFieldFocusObserver() override;

  bool IsObservingAutofillManagerForTesting() const {
    return autofill_manager_observation_.IsObserving();
  }

  // autofill::AutofillManager::Observer:
  void OnAutofillManagerStateChanged(
      autofill::AutofillManager& manager,
      autofill::AutofillManager::LifecycleState previous_state,
      autofill::AutofillManager::LifecycleState new_state) override;
  void OnAfterFocusOnFormField(autofill::AutofillManager& manager,
                               autofill::FormGlobalId /*form*/,
                               autofill::FieldGlobalId field) override;

 private:
  const base::RepeatingClosure credit_card_field_focused_callback_;
  const CreditCardFieldClassifier credit_card_field_classifier_;

  base::ScopedObservation<autofill::AutofillManager,
                          autofill::AutofillManager::Observer>
      autofill_manager_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_CREDIT_CARD_FIELD_FOCUS_OBSERVER_H_
