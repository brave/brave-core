/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/download/brave_download_item_view.h"

#include <algorithm>
#include <utility>

#include "base/auto_reset.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/download/download_item_mode.h"
#include "chrome/browser/ui/views/download/download_shelf_view.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "content/public/common/origin_util.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/styled_label.h"

using download::DownloadItem;

namespace {

constexpr int kTextWidth = 140;

// Padding before the icon and at end of the item.
constexpr int kStartPadding = 12;

// Horizontal padding between progress indicator and filename/status text.
constexpr int kProgressTextPadding = 8;

// Size of the space used for the progress indicator.
constexpr int kProgressIndicatorSize = 25;

// The minimum vertical padding above and below contents of the download item.
constexpr int kMinimumVerticalPadding = 2;

// The normal height of the item which may be exceeded if text is large.
constexpr int kDefaultHeight = 48;

// Lock icon color.
constexpr SkColor kDownloadUnlockIconColor = SkColorSetRGB(0xC6, 0x36, 0x26);

// Decrement of lock icon height from font baseline
constexpr int kDownloadUnlockIconHeightDecr = 1;

}  // namespace

BraveDownloadItemView::BraveDownloadItemView(
    DownloadUIModel::DownloadUIModelPtr download,
    DownloadShelfView* parent,
    views::View* accessible_alert)
    : DownloadItemView(std::move(download), parent, accessible_alert),
      brave_model_(model_.get()),
      is_origin_url_secure_(false),
      is_origin_url_visible_(false) {
  // Prepare origin url font.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  origin_url_font_list_ =
      rb.GetFontList(ui::ResourceBundle::BaseFont).DeriveWithSizeDelta(-1);
}

BraveDownloadItemView::~BraveDownloadItemView() = default;

// View overrides.

gfx::Size BraveDownloadItemView::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Call base class to get the width.
  gfx::Size size = DownloadItemView::CalculatePreferredSize(available_size);
  // Calculate the height accounting for the extra line.
  int child_height = file_name_label_->GetLineHeight() +
                     kBraveVerticalTextPadding +
                     origin_url_font_list_.GetHeight() +
                     kBraveVerticalTextPadding + status_label_->GetLineHeight();
  if (IsShowingWarningDialog()) {
    child_height = std::max(
        {child_height, GetButtonSize().height(), GetIcon().Size().width()});
  }
  size.set_height(
      std::max(kDefaultHeight, 2 * kMinimumVerticalPadding + child_height));
  return size;
}

void BraveDownloadItemView::OnPaint(gfx::Canvas* canvas) {
  DownloadItemView::OnPaint(canvas);
  if (is_origin_url_visible_)
    DrawOriginURL(canvas);
}

// download::DownloadItem::Observer overrides.

void BraveDownloadItemView::OnDownloadUpdated() {
  // Check for conditions that would disregard origin url change and fall back
  // onto base implementation to handle them.
  if (!model()->ShouldShowInShelf() ||
      (model_->GetState() == DownloadItem::COMPLETE &&
       model()->ShouldRemoveFromShelfWhenComplete())) {
    DownloadItemView::OnDownloadUpdated();
    return;
  }

  if (IsShowingWarningDialog() != model()->IsDangerous()) {
    DownloadItemView::OnDownloadUpdated();
  } else {
    // Update origin url first so that if the base class triggers paint fast
    // enough the new origin url is used and UpdateAccessibleName can use it
    // as well.
    bool needs_repaint = false;
    bool new_is_secure = false;
    std::u16string new_origin_url =
        brave_model_.GetOriginURLText(&new_is_secure);
    if (new_origin_url != origin_url_text_ ||
      new_is_secure != is_origin_url_secure_) {
      origin_url_text_ = new_origin_url;
      is_origin_url_secure_ = new_is_secure;
      needs_repaint = true;
    }

    DownloadItemView::OnDownloadUpdated();

    // Don't know if the base implementation triggered a repaint so trigger it
    // ourselves if we need to.
    if (needs_repaint)
      SchedulePaint();
  }

  // Update tooltip.
  std::u16string new_tip = brave_model_.GetTooltipText();
  if (new_tip != tooltip_text_) {
    tooltip_text_ = new_tip;
    TooltipTextChanged();
  }
}

std::u16string BraveDownloadItemView::CalculateAccessibleName() const {
  auto accessible_name = DownloadItemView::CalculateAccessibleName();
  if (!origin_url_text_.empty()) {
    std::u16string extra;
    if (!is_origin_url_secure_) {
      extra += char16_t(' ') + brave_l10n::GetLocalizedResourceUTF16String(
                                   IDS_NOT_SECURE_VERBOSE_STATE);
    }
    extra += char16_t(' ') + origin_url_text_;
    accessible_name += extra;
  }

  return accessible_name;
}

// Positioning routines.

int BraveDownloadItemView::GetYForFilenameText() const {
  int text_height = file_name_label_->GetLineHeight();
  if (!origin_url_text_.empty())
    text_height +=
        kBraveVerticalTextPadding + origin_url_font_list_.GetHeight();
  if (status_label_ && !status_label_->GetText().empty())
    text_height += kBraveVerticalTextPadding + status_label_->GetLineHeight();
  return (height() - text_height) / 2;
}

int BraveDownloadItemView::GetYForOriginURLText() const {
  return file_name_label_->y() + kBraveVerticalTextPadding;
}

int BraveDownloadItemView::GetYForStatusText() const {
  int y = GetYForOriginURLText();
  if (!origin_url_text_.empty())
    y += (origin_url_font_list_.GetHeight() + kBraveVerticalTextPadding);
  return y;
}

// Drawing routines.

void BraveDownloadItemView::DrawOriginURL(gfx::Canvas* canvas) {
  if (origin_url_text_.empty() || IsShowingWarningDialog())
    return;

  int x = kStartPadding + kProgressIndicatorSize + kProgressTextPadding;
  int text_width = kTextWidth;

  if (!is_origin_url_secure_) {
    DrawLockIcon(canvas);
    int dx = origin_url_font_list_.GetBaseline() + kOriginURLIconRightPadding;
    x += dx;
    text_width -= dx;
  }

  std::u16string originURL = gfx::ElideText(
      origin_url_text_, origin_url_font_list_, text_width, gfx::ELIDE_TAIL);
  int mirrored_x = GetMirroredXWithWidthInView(x, text_width);

  SkColor dimmed_text_color = SkColorSetA(
      GetColorProvider()->GetColor(ThemeProperties::COLOR_BOOKMARK_TEXT), 0xC7);
  canvas->DrawStringRect(
      originURL, origin_url_font_list_, dimmed_text_color,
      gfx::Rect(mirrored_x, GetYForOriginURLText(), text_width,
                origin_url_font_list_.GetHeight()));
}

void BraveDownloadItemView::DrawLockIcon(gfx::Canvas* canvas) {
  if (origin_url_text_.empty() || IsShowingWarningDialog())
    return;

  int mirrored_x = GetMirroredXWithWidthInView(
      kStartPadding + kProgressIndicatorSize + kProgressTextPadding,
      kTextWidth);

  // Get lock icon of the needed height.
  int dy = origin_url_font_list_.GetBaseline() - kDownloadUnlockIconHeightDecr;
  int y = GetYForOriginURLText() + (origin_url_font_list_.GetHeight() - dy) / 2;
  canvas->DrawImageInt(GetLockIcon(dy), mirrored_x, y);
}

// Get lock icon from vector icons.
gfx::ImageSkia BraveDownloadItemView::GetLockIcon(int height) {
  return gfx::CreateVectorIcon(kDownloadUnlockIcon, height,
    kDownloadUnlockIconColor);
}

void BraveDownloadItemView::UpdateLabels() {
  DownloadItemView::UpdateLabels();
  // Update visibility to avoid artifacts to be shown from upstream
  file_name_label_->SetVisible(
      !is_origin_url_visible_ &&
      (GetMode() == download::DownloadItemMode::kNormal));
}

void BraveDownloadItemView::SetOriginUrlVisible(bool visible) {
  is_origin_url_visible_ = visible;
  UpdateLabels();
}

void BraveDownloadItemView::OnMouseEntered(const ui::MouseEvent& event) {
  SetOriginUrlVisible(true);
}

void BraveDownloadItemView::OnMouseExited(const ui::MouseEvent& event) {
  SetOriginUrlVisible(false);
}

void BraveDownloadItemView::OnViewFocused(View* observed_view) {
  SetOriginUrlVisible(true);
}

void BraveDownloadItemView::OnViewBlurred(View* observed_view) {
  SetOriginUrlVisible(false);
}

BEGIN_METADATA(BraveDownloadItemView)
END_METADATA
