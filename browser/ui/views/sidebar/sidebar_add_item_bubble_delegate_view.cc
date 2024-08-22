/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"

#include <memory>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"
#include "url/gurl.h"

namespace {

constexpr gfx::Size kAddItemBubbleEntrySize{242, 40};

sidebar::SidebarService* GetSidebarService(Browser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

class SidebarAddItemButton : public views::LabelButton {
  METADATA_HEADER(SidebarAddItemButton, views::LabelButton)
 public:
  // Get theme provider to use browser's theme color in this dialog.
  SidebarAddItemButton(bool bold, const ui::ColorProvider* color_provider)
      : color_provider_(color_provider) {
    constexpr auto kDefaultAddItemInsets = gfx::Insets::TLBR(10, 34, 4, 8);
    SetBorder(views::CreateEmptyBorder(kDefaultAddItemInsets));
    if (color_provider_) {
      SetTextColor(
          views::Button::STATE_NORMAL,
          color_provider_->GetColor(kColorSidebarAddBubbleItemTextNormal));
      SetTextColor(
          views::Button::STATE_HOVERED,
          color_provider_->GetColor(kColorSidebarAddBubbleItemTextHovered));
      SetTextColor(
          views::Button::STATE_PRESSED,
          color_provider_->GetColor(kColorSidebarAddBubbleItemTextHovered));
    }

    const int size_diff = 13 - views::Label::GetDefaultFontList().GetFontSize();
    label()->SetFontList(
        views::Label::GetDefaultFontList()
            .DeriveWithSizeDelta(size_diff)
            .DeriveWithWeight(bold ? gfx::Font::Weight::SEMIBOLD
                                   : gfx::Font::Weight::NORMAL));
  }

  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override {
    return kAddItemBubbleEntrySize;
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    if (GetState() == STATE_HOVERED) {
      cc::PaintFlags flags;
      flags.setAntiAlias(true);
      flags.setStyle(cc::PaintFlags::kFill_Style);
      if (color_provider_) {
        flags.setColor(color_provider_->GetColor(
            kColorSidebarAddBubbleItemTextBackgroundHovered));
      }

      constexpr int kItemRadius = 6;
      // Fill the background.
      canvas->DrawRoundRect(GetLocalBounds(), kItemRadius, flags);
    }
  }

 private:
  const raw_ptr<const ui::ColorProvider> color_provider_;
};

BEGIN_METADATA(SidebarAddItemButton)
END_METADATA

}  // namespace

// static
views::Widget* SidebarAddItemBubbleDelegateView::Create(
    BraveBrowser* browser,
    views::View* anchor_view) {
  auto* delegate = new SidebarAddItemBubbleDelegateView(browser, anchor_view);
  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(delegate);
  auto* frame_view = delegate->GetBubbleFrameView();
  frame_view->bubble_border()->set_md_shadow_elevation(
      ChromeLayoutProvider::Get()->GetShadowElevationMetric(
          views::Emphasis::kHigh));
  frame_view->SetDisplayVisibleArrow(true);
  delegate->set_adjust_if_offscreen(true);
  delegate->SizeToContents();
  frame_view->SetCornerRadius(4);

  return bubble;
}

SidebarAddItemBubbleDelegateView::SidebarAddItemBubbleDelegateView(
    BraveBrowser* browser,
    views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view,
                               views::BubbleBorder::LEFT_TOP,
                               views::BubbleBorder::STANDARD_SHADOW),
      browser_(browser) {
  DCHECK(browser_);

  set_margins(gfx::Insets());
  set_title_margins(gfx::Insets());
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));

  if (const ui::ColorProvider* color_provider =
          BrowserView::GetBrowserViewForBrowser(browser_)->GetColorProvider()) {
    set_color(color_provider->GetColor(kColorSidebarAddBubbleBackground));
  }
  AddChildViews();
}

SidebarAddItemBubbleDelegateView::~SidebarAddItemBubbleDelegateView() = default;

void SidebarAddItemBubbleDelegateView::AddChildViews() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical));

  // |site_part| includes Add item header and current tab url.
  views::View* site_part = AddChildView(std::make_unique<views::View>());
  site_part->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets::TLBR(4, 4, 6, 4),
      6));
  // Use 13px font size.
  const int size_diff = 13 - views::Label::GetDefaultFontList().GetFontSize();
  views::Label::CustomFont font = {
      views::Label::GetDefaultFontList()
          .DeriveWithSizeDelta(size_diff)
          .DeriveWithWeight(gfx::Font::Weight::SEMIBOLD)};
  auto* header = site_part->AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ADD_ITEM_BUBBLE_TITLE),
      font));
  const ui::ColorProvider* color_provider =
      BrowserView::GetBrowserViewForBrowser(browser_)->GetColorProvider();
  if (color_provider) {
    header->SetEnabledColor(
        color_provider->GetColor(kColorSidebarAddBubbleHeaderText));
  }
  header->SetAutoColorReadabilityEnabled(false);
  constexpr auto kHeaderInsets = gfx::Insets::TLBR(10, 34, 4, 8);
  header->SetBorder(views::CreateEmptyBorder(kHeaderInsets));
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  if (sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    auto* button = site_part->AddChildView(
        std::make_unique<SidebarAddItemButton>(true, color_provider));
    const GURL active_tab_url = browser_->tab_strip_model()
                                    ->GetActiveWebContents()
                                    ->GetLastCommittedURL();
    DCHECK(active_tab_url.is_valid());
    auto button_label = active_tab_url.host();
    if (button_label.empty()) {
      button_label = active_tab_url.spec();
    }
    button->SetText(base::UTF8ToUTF16(button_label));
    button->SetCallback(base::BindRepeating(
        &SidebarAddItemBubbleDelegateView::OnCurrentItemButtonPressed,
        base::Unretained(this)));
  }

  const auto hidden_default_items =
      GetSidebarService(browser_)->GetHiddenDefaultSidebarItems();
  if (hidden_default_items.empty())
    return;

  auto* separator = AddChildView(std::make_unique<views::Separator>());
  if (color_provider) {
    separator->SetColorId(kColorSidebarSeparator);
  }

  // |default_part| includes hidden default items.
  views::View* default_part = AddChildView(std::make_unique<views::View>());
  default_part->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets::TLBR(6, 4, 8, 4),
      8));

  for (const auto& item : hidden_default_items) {
    auto* button = default_part->AddChildView(
        std::make_unique<SidebarAddItemButton>(false, color_provider));
    button->SetText(item.title);
    button->SetCallback(base::BindRepeating(
        &SidebarAddItemBubbleDelegateView::OnDefaultItemsButtonPressed,
        base::Unretained(this), item));
  }
}

void SidebarAddItemBubbleDelegateView::OnDefaultItemsButtonPressed(
    const sidebar::SidebarItem& item) {
  GetSidebarService(browser_)->AddItem(item);
  CloseOrReLayoutAfterAddingItem();
}

void SidebarAddItemBubbleDelegateView::OnCurrentItemButtonPressed() {
  browser_->sidebar_controller()->AddItemWithCurrentTab();
  CloseOrReLayoutAfterAddingItem();
}

void SidebarAddItemBubbleDelegateView::CloseOrReLayoutAfterAddingItem() {
  // Close this bubble when there is no item candidate for adding.
  if (GetSidebarService(browser_)->GetHiddenDefaultSidebarItems().empty() &&
      !sidebar::CanAddCurrentActiveTabToSidebar(browser_)) {
    GetWidget()->CloseWithReason(views::Widget::ClosedReason::kUnspecified);
    return;
  }

  // Otherwise, relayout with candidates for adding.
  RemoveAllChildViews();
  AddChildViews();
  GetWidget()->SetSize(GetWidget()->non_client_view()->GetPreferredSize());
}

BEGIN_METADATA(SidebarAddItemBubbleDelegateView)
END_METADATA
