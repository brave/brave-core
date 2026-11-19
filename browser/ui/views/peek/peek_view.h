/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/view.h"

class Profile;
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
//
// The card is centered horizontally and its top stays attached to the toolbar.
class PeekView : public views::View {
  METADATA_HEADER(PeekView, views::View)

 public:
  DECLARE_CLASS_ELEMENT_IDENTIFIER_VALUE(kCardElementId);
  DECLARE_CLASS_ELEMENT_IDENTIFIER_VALUE(kCloseButtonElementId);

  class Delegate {
   public:
    virtual void OnPeekViewCloseRequested() = 0;

   protected:
    virtual ~Delegate() = default;
  };

  PeekView(Delegate& delegate, Profile& profile);
  PeekView(const PeekView&) = delete;
  PeekView& operator=(const PeekView&) = delete;
  ~PeekView() override;

  // Shows `contents` in the card. Does not take ownership, so the caller must
  // keep `contents` alive while it is shown. Pass nullptr to detach the page
  // before destroying it.
  void SetWebContents(content::WebContents* contents);

  // Refreshes the favicon, title and host from the current
  // contents. The toolbar does not observe the page, so call this whenever the
  // contents change.
  void UpdateToolbar();

  // Moves keyboard focus into the page.
  void FocusContents();

  // Scales the card up from the toolbar and fades in the scrim. Call before
  // the widget is shown.
  void AnimateOpen();

  views::View* card_for_testing() { return card_; }
  views::ImageButton* close_button_for_testing() { return close_button_; }
  views::Label* title_for_testing() { return title_; }
  views::Label* host_for_testing() { return host_; }

  // views::View:
  void Layout(PassKey) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  void OnThemeChanged() override;

 private:
  // The card's size in `bounds`.
  gfx::Size GetCardSize(const gfx::Rect& bounds) const;

  // Scales the card, and its shadow, up from the top edge using layer
  // animations, so the page is not re-laid out on every frame.
  void StartCardOpenAnimation();

  // Set by `AnimateOpen()` until the first layout gives the card its bounds.
  bool open_animation_pending_ = false;

  const raw_ref<Delegate> delegate_;

  std::unique_ptr<ViewShadow> card_shadow_;

  raw_ptr<views::View> scrim_ = nullptr;
  raw_ptr<views::View> card_ = nullptr;
  raw_ptr<views::ImageView> favicon_ = nullptr;
  raw_ptr<views::Label> title_ = nullptr;
  raw_ptr<views::Label> host_ = nullptr;
  raw_ptr<views::ImageButton> close_button_ = nullptr;
  raw_ptr<views::WebView> web_view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PEEK_PEEK_VIEW_H_
