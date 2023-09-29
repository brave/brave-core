/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_H_

#include <string>

#include "ui/views/widget/widget.h"

namespace autofill {

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
  virtual void OnBubbleClosed(views::Widget::ClosedReason closed_reason) = 0;
};

}  // namespace autofill

#endif  // BRAVE_BROWSER_UI_AUTOFILL_CONFIRM_AUTOCOMPLETE_BUBBLE_CONTROLLER_H_
