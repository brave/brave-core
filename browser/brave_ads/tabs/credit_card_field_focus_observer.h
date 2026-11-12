/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TABS_CREDIT_CARD_FIELD_FOCUS_OBSERVER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TABS_CREDIT_CARD_FIELD_FOCUS_OBSERVER_H_

#include "base/functional/callback.h"
#include "base/scoped_observation.h"
#include "components/autofill/core/browser/foundations/autofill_manager.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave_ads {

// A checkout page's order value is often not yet rendered at `load` time,
// but is reliably present once the user focuses a payment field. Reads
// Autofill's own field classification (`AutofillField::Type()`), rather
// than a self-owned DOM heuristic, so this also catches merchants that
// never set `autocomplete="cc-*"`.
class CreditCardFieldFocusObserver final
    : public autofill::AutofillManager::Observer {
 public:
  // Classifies a focused field as a credit card field or not. Defaults to
  // the real `IsCreditCardField()`, which reads Autofill's own cached
  // `FormStructure`; injectable so tests can verify this class's own
  // observer-to-callback wiring without depending on that cache, which
  // requires a full, real end-to-end Autofill parse to populate.
  using CreditCardFieldClassifier =
      base::RepeatingCallback<bool(autofill::AutofillManager&,
                                   autofill::FieldGlobalId)>;

  CreditCardFieldFocusObserver(
      content::WebContents& web_contents,
      base::RepeatingClosure credit_card_field_focused_callback);
  CreditCardFieldFocusObserver(
      content::WebContents& web_contents,
      base::RepeatingClosure credit_card_field_focused_callback,
      CreditCardFieldClassifier credit_card_field_classifier);
  ~CreditCardFieldFocusObserver() override;

  CreditCardFieldFocusObserver(const CreditCardFieldFocusObserver&) = delete;
  CreditCardFieldFocusObserver& operator=(const CreditCardFieldFocusObserver&) =
      delete;

  // autofill::AutofillManager::Observer:
  void OnAutofillManagerStateChanged(
      autofill::AutofillManager& manager,
      autofill::AutofillManager::LifecycleState previous_state,
      autofill::AutofillManager::LifecycleState new_state) override;
  void OnAfterFocusOnFormField(autofill::AutofillManager& manager,
                               autofill::FormGlobalId form,
                               autofill::FieldGlobalId field) override;

 private:
  const base::RepeatingClosure credit_card_field_focused_callback_;
  const CreditCardFieldClassifier credit_card_field_classifier_;

  base::ScopedObservation<autofill::AutofillManager,
                          autofill::AutofillManager::Observer>
      autofill_manager_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TABS_CREDIT_CARD_FIELD_FOCUS_OBSERVER_H_
