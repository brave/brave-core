// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_strings.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace {
class BraveShieldsActionViewHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  SkPath GetHighlightPath(const views::View* view) override {
    return static_cast<const BraveShieldsActionView*>(view)->GetHighlightPath();
  }
};
}  // namespace

DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(BraveShieldsActionView,
                                      kShieldsActionIcon);

BraveShieldsActionView::BraveShieldsActionView(
    BrowserWindowInterface* browser_window_interface,
    views::LayoutProvider& layout_provider,
    CreateWebUIBubbleManagerCallback create_bubble_manager_callback)
    : LabelButton(views::Button::PressedCallback(), std::u16string()),
      browser_window_interface_(browser_window_interface),
      controller_(std::make_unique<BraveShieldsActionController>(
          browser_window_interface,
          std::move(create_bubble_manager_callback))),
      layout_provider_(layout_provider) {
  controller_->SetOnStateChanged(
      base::BindRepeating(&BraveShieldsActionView::OnControllerStateChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  controller_->SetAnchorView(this);

  SetAccessibleName(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetProperty(views::kElementIdentifierKey, kShieldsActionIcon);

  // The MenuButtonController makes sure the panel closes when clicked if the
  // panel is already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveShieldsActionView::ButtonPressed,
                          weak_ptr_factory_.GetWeakPtr()),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  SetButtonController(std::move(menu_button_controller));
}

void BraveShieldsActionView::ButtonPressed() {
  controller_->OnButtonPressed();
}

BraveShieldsActionView::~BraveShieldsActionView() = default;

void BraveShieldsActionView::OnControllerStateChanged() {
  Update();
}

void BraveShieldsActionView::Init() {
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveShieldsActionViewHighlightPathGenerator>());
  Update();
}

SkPath BraveShieldsActionView::GetHighlightPath() const {
  // Set the highlight path for the toolbar button, making it inset so that the
  // badge can show outside it in the fake margin on the right that we create.
  auto highlight_insets =
      gfx::Insets::TLBR(0, 0, 0, -1 * kBraveActionLeftMarginExtra);
  gfx::Rect rect(GetPreferredSize());
  rect.Inset(highlight_insets);
  const int radii = layout_provider_->GetCornerRadiusMetric(
      views::Emphasis::kHigh, rect.size());
  return SkPath::RRect(gfx::RectToSkRect(rect), radii, radii);
}

views::Widget* BraveShieldsActionView::GetBubbleWidget() {
  return controller_->GetBubbleWidget();
}

void BraveShieldsActionView::Update() {
  controller_->RefreshButtonImages(this);
}

std::unique_ptr<views::LabelButtonBorder>
BraveShieldsActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets::TLBR(0, 0, 0, 0));
  return border;
}

std::u16string BraveShieldsActionView::GetRenderedTooltipText(
    const gfx::Point& p) const {
  return controller_->GetTooltipText();
}

void BraveShieldsActionView::OnThemeChanged() {
  LabelButton::OnThemeChanged();

  const auto* const color_provider = GetColorProvider();
  if (!color_provider) {
    return;
  }

  // Apply same ink drop effect with location bar's other icon views.
  auto* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  SetHasInkDropActionOnClick(true);
  views::InkDrop::Get(this)->SetVisibleOpacity(kOmniboxOpacitySelected);
  views::InkDrop::Get(this)->SetHighlightOpacity(kOmniboxOpacityHovered);
  ink_drop->SetBaseColor(color_provider->GetColor(kColorOmniboxText));
}

BEGIN_METADATA(BraveShieldsActionView)
END_METADATA
