// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_DELEGATE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_DELEGATE_VIEW_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

class BraveHelpBubbleDelegateView : public views::BubbleDialogDelegateView {
 public:
  METADATA_HEADER(BraveHelpBubbleDelegateView);

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override = default;

    virtual void OnBubbleDestroying(views::Widget* widget) {}
  };
  explicit BraveHelpBubbleDelegateView(View* anchor_view,
                                       const std::string& text);
  BraveHelpBubbleDelegateView(const BraveHelpBubbleDelegateView&) = delete;
  BraveHelpBubbleDelegateView& operator=(const BraveHelpBubbleDelegateView&) =
      delete;
  ~BraveHelpBubbleDelegateView() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void Show();
  void Hide();

 private:
  // views::BubbleDialogDelegate
  std::unique_ptr<views::NonClientFrameView> CreateNonClientFrameView(
      views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  void SetUpLabel(views::Label* label,
                  const std::u16string& text,
                  int font_size,
                  gfx::Font::Weight font_weight);

  base::ObserverList<Observer> observers_;
  raw_ptr<views::Widget> widget_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_BRAVE_HELP_BUBBLE_DELEGATE_VIEW_H_
