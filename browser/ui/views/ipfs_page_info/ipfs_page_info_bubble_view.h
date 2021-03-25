/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_IPFS_PAGE_INFO_IPFS_PAGE_INFO_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_IPFS_PAGE_INFO_IPFS_PAGE_INFO_BUBBLE_VIEW_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/reputation/safety_tip_ui.h"
#include "chrome/browser/ui/page_info/page_info_dialog.h"
#include "chrome/browser/ui/views/bubble_anchor_util_views.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "chrome/browser/ui/views/page_info/chosen_object_view_observer.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view_base.h"
#include "chrome/browser/ui/views/page_info/page_info_hover_button.h"
#include "chrome/browser/ui/views/page_info/permission_selector_row.h"
#include "chrome/browser/ui/views/page_info/permission_selector_row_observer.h"
#include "components/page_info/page_info_ui.h"
#include "components/safe_browsing/buildflags.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

class GURL;
class Profile;

namespace content {
class WebContents;
}  // namespace content

namespace views {
class View;
class ImageView;
class StyledLabel;
}  // namespace views

// The IpfsPageInfoBubbleView shows information about IPFS pages.
class IpfsPageInfoBubbleView : public views::BubbleDialogDelegateView,
                               public content::WebContentsObserver {
 public:
  METADATA_HEADER(IpfsPageInfoBubbleView);
  // If |anchor_view| is nullptr, or has no Widget, |parent_window| may be
  // provided to ensure this bubble is closed when the parent closes.
  IpfsPageInfoBubbleView(views::View* anchor_view,
                         const gfx::Rect& anchor_rect,
                         gfx::NativeView parent_window,
                         content::WebContents* web_contents,
                         const GURL& url);
  IpfsPageInfoBubbleView(const IpfsPageInfoBubbleView&) = delete;
  IpfsPageInfoBubbleView& operator=(const IpfsPageInfoBubbleView&) = delete;
  ~IpfsPageInfoBubbleView() override;

  // WebContentsObserver:
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void DidStartNavigation(content::NavigationHandle* handle) override;
  void DidChangeVisibleSecurityState() override;

  void SettingsLinkClicked(const ui::Event& event);
  void LearnMoreClicked(const ui::Event& event);

  static views::BubbleDialogDelegateView* CreatePageInfoBubble(
      views::View* anchor_view,
      const gfx::Rect& anchor_rect,
      gfx::NativeWindow parent_window,
      Profile* profile,
      content::WebContents* web_contents,
      const GURL& url,
      PageInfoClosingCallback closing_callback);

 private:
  void FillEmptyCell(views::GridLayout* layout);
  void AddTitleLabel(views::GridLayout* layout);
  void AddBodyText(views::GridLayout* layout);
  void AddExclamationIcon(views::GridLayout* layout);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_IPFS_PAGE_INFO_IPFS_PAGE_INFO_BUBBLE_VIEW_H_
