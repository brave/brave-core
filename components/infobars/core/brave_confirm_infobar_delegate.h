/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_INFOBARS_CORE_BRAVE_CONFIRM_INFOBAR_DELEGATE_H_
#define BRAVE_COMPONENTS_INFOBARS_CORE_BRAVE_CONFIRM_INFOBAR_DELEGATE_H_

#include <string>
#include <vector>

#include "components/infobars/core/confirm_infobar_delegate.h"

class BraveConfirmInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  BraveConfirmInfoBarDelegate(const BraveConfirmInfoBarDelegate&) = delete;
  BraveConfirmInfoBarDelegate& operator=(const BraveConfirmInfoBarDelegate&) =
      delete;
  ~BraveConfirmInfoBarDelegate() override;

  virtual bool HasCheckbox() const;
  virtual std::u16string GetCheckboxText() const;
  virtual void SetCheckboxChecked(bool checked);
  // Returns true when delegate wants to intercept closing.
  // Then closing will be cancelled and delegate should remove infobar
  // after doing something.
  virtual bool InterceptClosing();
  virtual std::vector<int> GetButtonsOrder() const;
  virtual bool IsProminent(int id) const;
  virtual bool ExtraButtonPressed();

  int GetButtons() const override;

 protected:
  BraveConfirmInfoBarDelegate();
};

#endif  // BRAVE_COMPONENTS_INFOBARS_CORE_BRAVE_CONFIRM_INFOBAR_DELEGATE_H_
