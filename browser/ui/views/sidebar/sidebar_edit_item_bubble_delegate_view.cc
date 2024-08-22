/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_edit_item_bubble_delegate_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/sidebar/browser/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/color/color_provider.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/box_layout.h"

namespace {
sidebar::SidebarService* GetSidebarService(Browser* browser) {
  return sidebar::SidebarServiceFactory::GetForProfile(browser->profile());
}

gfx::FontList GetFont(int font_size, gfx::Font::Weight weight) {
  gfx::FontList font_list;
  return font_list.DeriveWithSizeDelta(font_size - font_list.GetFontSize())
      .DeriveWithWeight(weight);
}
}  // namespace

// static
views::Widget* SidebarEditItemBubbleDelegateView::Create(
    BraveBrowser* browser,
    const sidebar::SidebarItem& item,
    views::View* anchor_view) {
  auto* delegate =
      new SidebarEditItemBubbleDelegateView(browser, item, anchor_view);
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

SidebarEditItemBubbleDelegateView::SidebarEditItemBubbleDelegateView(
    BraveBrowser* browser,
    const sidebar::SidebarItem& item,
    views::View* anchor_view)
    : BubbleDialogDelegateView(anchor_view,
                               views::BubbleBorder::LEFT_TOP,
                               views::BubbleBorder::STANDARD_SHADOW),
      target_item_(item),
      browser_(browser) {
  SetAcceptCallback(base::BindOnce(
      &SidebarEditItemBubbleDelegateView::UpdateItem, base::Unretained(this)));
}

SidebarEditItemBubbleDelegateView::~SidebarEditItemBubbleDelegateView() =
    default;

void SidebarEditItemBubbleDelegateView::AddChildViews() {
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(12),
      /* between_child_spacing = */ 10));
  views::Label::CustomFont header_font = {
      GetFont(16, gfx::Font::Weight::NORMAL)};
  auto* header = AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_EDIT_ITEM_BUBBLE_HEADER),
      header_font));
  const ui::ColorProvider* color_provider =
      BrowserView::GetBrowserViewForBrowser(browser_)->GetColorProvider();
  DCHECK(color_provider);
  header->SetEnabledColor(
      color_provider->GetColor(kColorSidebarAddBubbleHeaderText));
  header->SetAutoColorReadabilityEnabled(false);
  header->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  auto* title_part = AddChildView(std::make_unique<views::View>());
  title_part->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      /* between_child_spacing = */ 4));
  views::Label::CustomFont title_font = {
      GetFont(13, gfx::Font::Weight::NORMAL)};
  auto* title = title_part->AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_EDIT_ITEM_BUBBLE_TITLE),
      title_font));
  title->SetEnabledColor(
      color_provider->GetColor(kColorSidebarAddBubbleHeaderText));
  title->SetAutoColorReadabilityEnabled(false);
  title->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  title_tf_ = title_part->AddChildView(std::make_unique<views::Textfield>());
  title_tf_->SetController(this);
  title_tf_->SetText(target_item_.title);
  title_tf_->SelectAll(true);
  title_tf_->SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SIDEBAR_EDIT_ITEM_BUBBLE_AX_TITLE_EDITOR_LABEL));

  auto* url_part = AddChildView(std::make_unique<views::View>());
  url_part->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical, gfx::Insets(),
      /* between_child_spacing = */ 4));
  auto* url = url_part->AddChildView(std::make_unique<views::Label>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_EDIT_ITEM_BUBBLE_URL),
      title_font));
  url->SetEnabledColor(
      color_provider->GetColor(kColorSidebarAddBubbleHeaderText));
  url->SetAutoColorReadabilityEnabled(false);
  url->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  url_tf_ = url_part->AddChildView(std::make_unique<views::Textfield>());
  url_tf_->SetController(this);
  url_tf_->SetText(base::UTF8ToUTF16(target_item_.url.spec()));
  url_tf_->SelectAll(true);
  url_tf_->SetAccessibleName(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SIDEBAR_EDIT_ITEM_BUBBLE_AX_URL_EDITOR_LABEL));

  UpdateOKButtonEnabledState();
}

views::View* SidebarEditItemBubbleDelegateView::GetInitiallyFocusedView() {
  DCHECK(title_tf_);
  return title_tf_;
}

void SidebarEditItemBubbleDelegateView::AddedToWidget() {
  AddChildViews();
}

void SidebarEditItemBubbleDelegateView::ContentsChanged(
    views::Textfield* sender,
    const std::u16string& new_contents) {
  UpdateOKButtonEnabledState();
}

void SidebarEditItemBubbleDelegateView::UpdateItem() {
  std::u16string new_title = title_tf_->GetText();
  if (new_title.empty())
    new_title = url_tf_->GetText();

  GURL new_url(url_tf_->GetText());
  if (new_url.is_empty())
    new_url = target_item_.url;

  GetSidebarService(browser_)->UpdateItem(target_item_.url, new_url,
                                          target_item_.title, new_title);
}

void SidebarEditItemBubbleDelegateView::UpdateOKButtonEnabledState() {
  DCHECK(title_tf_ && url_tf_);

  // Update item only when url or title is changed.
  GURL new_url(url_tf_->GetText());
  if (new_url.is_empty())
    new_url = target_item_.url;

  const bool ok_button_enabled =
      target_item_.url != new_url || target_item_.title != title_tf_->GetText();

  SetButtonEnabled(ui::mojom::DialogButton::kOk, ok_button_enabled);
}

BEGIN_METADATA(SidebarEditItemBubbleDelegateView)
END_METADATA
