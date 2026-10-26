/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/animation/animation_delegate_views.h"
#include "ui/views/view.h"

class BrowserWindowInterface;
class PeekShieldsButton;
class PeekShieldsPanelProvider;
class ViewShadow;

namespace content {
class WebContents;
}  // namespace content

namespace views {
class ImageButton;
class ImageView;
class Label;
class WebView;
}  // namespace views

// Root view of the Peek widget. Dims the web contents area it covers and shows
// a card hanging from the browser toolbar, with a small toolbar of its own and
// the Peek web contents. Clicking outside the card dismisses Peek.
class PeekView : public views::View, public views::AnimationDelegateViews {
  METADATA_HEADER(PeekView, views::View)

 public:
  class Delegate {
   public:
    virtual void OnPeekViewCloseRequested() = 0;
    virtual void OnPeekViewOpenAsTabRequested() = 0;

   protected:
    virtual ~Delegate() = default;
  };

  PeekView(Delegate& delegate,
           BrowserWindowInterface& browser,
           PeekShieldsPanelProvider& shields_panel_provider);
  PeekView(const PeekView&) = delete;
  PeekView& operator=(const PeekView&) = delete;
  ~PeekView() override;

  void SetWebContents(content::WebContents* contents);
  void UpdateToolbar();
  void FocusContents();

  // Grows the card from the center and fades in the scrim. Call before the
  // widget is shown.
  void AnimateOpen();

  views::View* card_for_testing() { return card_; }
  gfx::SlideAnimation& open_animation_for_testing() { return open_animation_; }
  views::ImageButton* close_button_for_testing() { return close_button_; }
  views::ImageButton* open_as_tab_button_for_testing() {
    return open_as_tab_button_;
  }
  views::ImageButton* site_info_button_for_testing() {
    return site_info_button_;
  }
  PeekShieldsButton* shields_button_for_testing() { return shields_button_; }

  // views::View:
  void Layout(PassKey) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  void OnPaintBackground(gfx::Canvas* canvas) override;
  void OnThemeChanged() override;

  // views::AnimationDelegateViews:
  void AnimationProgressed(const gfx::Animation* animation) override;

 private:
  void ShowPageInfo();

  const raw_ref<Delegate> delegate_;
  const raw_ref<BrowserWindowInterface> browser_;

  raw_ptr<views::View> card_ = nullptr;
  raw_ptr<views::ImageButton> site_info_button_ = nullptr;
  raw_ptr<views::ImageView> favicon_ = nullptr;
  raw_ptr<views::Label> title_ = nullptr;
  raw_ptr<views::Label> host_ = nullptr;
  raw_ptr<PeekShieldsButton> shields_button_ = nullptr;
  raw_ptr<views::ImageButton> open_as_tab_button_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::WebView> web_view_ = nullptr;

  std::unique_ptr<ViewShadow> card_shadow_;
  gfx::SlideAnimation open_animation_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_VIEW_H_
