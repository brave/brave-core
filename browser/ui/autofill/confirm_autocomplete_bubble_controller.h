// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_H_

#include <string>

#include "components/autofill/core/browser/ui/payments/payments_bubble_closed_reasons.h"

namespace autofill {

class AutofillBubbleBase;

class ConfirmAutocompleteBubbleController {
 public:
  ConfirmAutocompleteBubbleController() = default;

  ConfirmAutocompleteBubbleController(
      const ConfirmAutocompleteBubbleController&) = delete;
  ConfirmAutocompleteBubbleController& operator=(
      const ConfirmAutocompleteBubbleController&) = delete;

  virtual ~ConfirmAutocompleteBubbleController() = default;

  virtual std::u16string GetWindowTitle() const = 0;

  virtual std::u16string GetAcceptButtonText() const = 0;
  virtual std::u16string GetDeclineButtonText() const = 0;

  virtual void OnAcceptButton() = 0;
  virtual void OnBubbleClosed(PaymentsBubbleClosedReason closed_reason) = 0;
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_H_
