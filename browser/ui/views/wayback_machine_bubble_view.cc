/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/wayback_machine_bubble_view.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/views/page_action/wayback_machine_action_icon_view.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace {

BraveWaybackMachineTabHelper* GetTabHelper(content::WebContents* web_contents) {
  if (!web_contents) {
    return nullptr;
  }

  return BraveWaybackMachineTabHelper::FromWebContents(web_contents);
}

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}

}  // namespace

// static
void WaybackMachineBubbleView::Show(Browser* browser, views::View* anchor) {
  auto* web_contents = browser->tab_strip_model()->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  auto* tab_helper = GetTabHelper(web_contents);
  if (!tab_helper) {
    return;
  }

  // Don't need to launch again if existed.
  if (tab_helper->active_window()) {
    return;
  }

  views::Widget* const widget = views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<WaybackMachineBubbleView>(web_contents->GetWeakPtr(),
                                                 anchor));
  widget->Show();
  tab_helper->set_active_window(widget->GetNativeWindow());
}

WaybackMachineBubbleView::WaybackMachineBubbleView(
    base::WeakPtr<content::WebContents> web_contents,
    views::View* anchor)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT),
      web_contents_(web_contents) {
  SetShowCloseButton(true);
  set_fixed_width(360);
  set_should_ignore_snapping(true);
  set_margins(gfx::Insets::TLBR(0, 24, 24, 24));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets*/ gfx::Insets(),
      /*between_child_spacing*/ 24));

  auto* tab_helper = GetTabHelper(web_contents_.get());
  CHECK(tab_helper);
  bool need_checking =
      tab_helper->wayback_state() == WaybackState::kNeedToCheck;

  // Header label.
  auto* label = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          need_checking
              ? IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_SORRY_HEADER_TEXT
              : IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_CANT_FIND_HEADER_TEXT)));
  label->SetFontList(GetFont(/*font_size*/ 16, gfx::Font::Weight::SEMIBOLD));
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Contents label.
  label = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          need_checking
              ? IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_ASK_ABOUT_CHECK_TEXT
              : IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_NOT_AVAILABLE_TEXT)));
  label->SetFontList(GetFont(/*font_size*/ 14, gfx::Font::Weight::SEMIBOLD));
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  // Only need buttons for checking wayback url.
  if (!need_checking) {
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
    return;
  }

  SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk) |
             static_cast<int>(ui::mojom::DialogButton::kCancel));
  SetButtonLabel(ui::mojom::DialogButton::kOk,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_CHECK_BUTTON_TEXT));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 brave_l10n::GetLocalizedResourceUTF16String(
                     IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_DISMISS_BUTTON_TEXT));

  // Unretained is safe beaause this button is owned by this class.
  SetAcceptCallback(base::BindRepeating(&WaybackMachineBubbleView::OnAccepted,
                                        base::Unretained(this)));
}

WaybackMachineBubbleView::~WaybackMachineBubbleView() {
  if (auto* tab_helper = GetTabHelper(web_contents_.get())) {
    tab_helper->set_active_window(nullptr);
  }
}

void WaybackMachineBubbleView::OnWidgetVisibilityChanged(views::Widget* widget,
                                                         bool visible) {
  BubbleDialogDelegateView::OnWidgetVisibilityChanged(widget, visible);

  // Use active icon color only when bubble is shown.
  static_cast<WaybackMachineActionIconView*>(GetAnchorView())
      ->SetActive(visible);
}

void WaybackMachineBubbleView::OnAccepted() {
  if (auto* tab_helper = GetTabHelper(web_contents_.get())) {
    tab_helper->FetchWaybackURL();
  }
}

BEGIN_METADATA(WaybackMachineBubbleView)
END_METADATA
