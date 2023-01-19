/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include "chrome/browser/ui/views/infobars/confirm_infobar.h"
#include "ui/base/metadata/metadata_header_macros.h"

namespace views {
class Checkbox;
}  // namespace views

// Add checkbox to ConfirmInfoBar.
// "Would you like to do X?  [Checkbox]       _Learn More_ [x]"
class BraveConfirmInfoBar : public ConfirmInfoBar {
 public:
  METADATA_HEADER(BraveConfirmInfoBar);
  explicit BraveConfirmInfoBar(
      std::unique_ptr<BraveConfirmInfoBarDelegate> delegate);
  BraveConfirmInfoBar(const BraveConfirmInfoBar&) = delete;
  BraveConfirmInfoBar& operator=(const BraveConfirmInfoBar&) = delete;
  ~BraveConfirmInfoBar() override;

  // ConfirmInfoBar overrides:
  void Layout() override;
  int NonLabelWidth() const override;
  void CloseButtonPressed() override;

 private:
  BraveConfirmInfoBarDelegate* GetBraveDelegate();

  void CheckboxPressed();

  raw_ptr<views::Checkbox> checkbox_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_CONFIRM_INFOBAR_H_
