/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_action/wayback_machine_bubble_view.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/actions/actions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/button/md_text_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr int kBubbleWidth = 432;
constexpr int kArchiveIconSize = 56;
constexpr int kPadding = 24;

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
void WaybackMachineBubbleView::Show(content::WebContents* web_contents,
                                    views::View* anchor,
                                    actions::ActionItem* item) {
  auto* tab_helper = GetTabHelper(web_contents);
  if (!tab_helper) {
    return;
  }

  // Don't need to launch again if existed.
  if (tab_helper->active_window().has_value()) {
    return;
  }

  views::Widget* const widget = views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<WaybackMachineBubbleView>(web_contents->GetWeakPtr(),
                                                 anchor, item));
  widget->Show();
  tab_helper->set_active_window(widget->GetNativeWindow());
}

WaybackMachineBubbleView::WaybackMachineBubbleView(
    base::WeakPtr<content::WebContents> web_contents,
    views::View* anchor,
    actions::ActionItem* item)
    : BubbleDialogDelegateView(anchor, views::BubbleBorder::TOP_RIGHT),
      web_contents_(web_contents),
      item_(item) {
  if (item_) {
    item_->SetIsShowingBubble(true);
  }
  SetShowCloseButton(true);
  set_fixed_width(kBubbleWidth);
  set_should_ignore_snapping(true);
  set_title_margins(gfx::Insets(kPadding));
  set_margins(gfx::Insets::TLBR(0, kPadding, kPadding, kPadding));

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets*/ gfx::Insets(),
      /*between_child_spacing*/ kPadding));

  auto* tab_helper = GetTabHelper(web_contents_.get());
  CHECK(tab_helper);
  const bool need_checking =
      tab_helper->wayback_state() == WaybackState::kNeedToCheck;

  SetTitle(l10n_util::GetStringUTF16(
      need_checking ? IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_SORRY_HEADER_TEXT
                    : IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_CANT_FIND_HEADER_TEXT));

  auto* content_row = AddChildView(std::make_unique<views::View>());
  auto* row_layout =
      content_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 16));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStart);

  auto* body = content_row->AddChildView(
      std::make_unique<views::Label>(l10n_util::GetStringUTF16(
          need_checking
              ? IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_ASK_ABOUT_CHECK_TEXT
              : IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_NOT_AVAILABLE_TEXT)));
  body->SetFontList(GetFont(/*font_size*/ 14, gfx::Font::Weight::NORMAL));
  body->SetMultiLine(true);
  body->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  row_layout->SetFlexForView(body, 1);

  auto* icon = content_row->AddChildView(std::make_unique<views::ImageView>());
  icon->SetImage(ui::ImageModel::FromVectorIcon(
      kLeoInternetArchiveIcon, nala::kColorIconDefault, kArchiveIconSize));

  if (!need_checking) {
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
    return;
  }

  SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk) |
             static_cast<int>(ui::mojom::DialogButton::kCancel));
  SetButtonLabel(ui::mojom::DialogButton::kOk,
                 l10n_util::GetStringUTF16(
                     IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_CHECK_BUTTON_TEXT));
  SetButtonLabel(ui::mojom::DialogButton::kCancel,
                 l10n_util::GetStringUTF16(
                     IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_DISMISS_BUTTON_TEXT));

  auto dont_ask_again = std::make_unique<views::MdTextButton>(
      base::BindRepeating(&WaybackMachineBubbleView::OnDontAskAgain,
                          base::Unretained(this)),
      l10n_util::GetStringUTF16(
          IDS_BRAVE_WAYBACK_MACHINE_BUBBLE_DONT_ASK_AGAIN_TEXT));
  dont_ask_again->SetStyle(ui::ButtonStyle::kText);
  SetExtraView(std::move(dont_ask_again));

  SetAcceptCallback(base::BindRepeating(&WaybackMachineBubbleView::OnAccepted,
                                        base::Unretained(this)));
}

WaybackMachineBubbleView::~WaybackMachineBubbleView() {
  if (auto* tab_helper = GetTabHelper(web_contents_.get())) {
    tab_helper->set_active_window(std::nullopt);
  }
  if (item_) {
    item_->SetIsShowingBubble(false);
  }
}

void WaybackMachineBubbleView::OnAccepted() {
  if (auto* tab_helper = GetTabHelper(web_contents_.get())) {
    tab_helper->FetchWaybackURL();
  }
}

void WaybackMachineBubbleView::OnDontAskAgain() {
  if (web_contents_) {
    auto* profile =
        Profile::FromBrowserContext(web_contents_->GetBrowserContext());
    profile->GetPrefs()->SetBoolean(kBraveWaybackMachineEnabled, false);
  }
  if (views::Widget* widget = GetWidget()) {
    widget->CloseWithReason(views::Widget::ClosedReason::kCancelButtonClicked);
  }
}

BEGIN_METADATA(WaybackMachineBubbleView)
END_METADATA
