/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_H_

#include "base/memory/weak_ptr.h"
#include "ui/views/window/dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

// TODO(sko): We may want this to be a bubble.
class AdBlockAdjustmentDialog : public views::DialogDelegateView {
 public:
  explicit AdBlockAdjustmentDialog(content::WebContents* contents);
  ~AdBlockAdjustmentDialog() override;

  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize() const override;
  void WindowClosing() override;

 private:
  void DisableAdBlockForSite();

  base::WeakPtr<content::WebContents> contents_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_PLAYER_AD_BLOCK_ADJUSTMENT_DIALOG_H_
