// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"  // NOLINT
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view.h"

namespace {
class BraveShieldsActionViewHighlightPathGenerator
    : public views::HighlightPathGenerator {
 public:
  BraveShieldsActionViewHighlightPathGenerator() = default;
  BraveShieldsActionViewHighlightPathGenerator(
      const BraveShieldsActionViewHighlightPathGenerator&) = delete;
  BraveShieldsActionViewHighlightPathGenerator& operator=(
      const BraveShieldsActionViewHighlightPathGenerator&) = delete;
  ~BraveShieldsActionViewHighlightPathGenerator() override = default;

  SkPath GetHighlightPath(const views::View* view) override {
    return static_cast<const BraveShieldsActionView*>(view)->GetHighlightPath();
  }
};
}  // namespace

BraveShieldsActionView::BraveShieldsActionView()
    : LabelButton(base::BindRepeating(&BraveShieldsActionView::ButtonPressed,
                                      base::Unretained(this)),
                  std::u16string()) {
  auto* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetBaseColorCallback(base::BindRepeating(
      [](views::View* host) { return GetToolbarInkDropBaseColor(host); },
      this));

  SetAccessibleName(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  SetHasInkDropActionOnClick(true);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);
}

BraveShieldsActionView::~BraveShieldsActionView() = default;

void BraveShieldsActionView::Init() {
  // Create badge-and-image source like an extension icon would
  auto preferred_size = GetPreferredSize();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  std::unique_ptr<IconWithBadgeImageSource> image_source(
      new BraveActionIconWithBadgeImageSource(preferred_size));
  // Set icon on badge using actual extension icon resource
  gfx::ImageSkia image;

  const SkBitmap bitmap =
      rb.GetImageNamed(IDR_BRAVE_SHIELDS_ICON_64).AsBitmap();
  float scale = static_cast<float>(bitmap.width()) / kBraveActionGraphicSize;
  image.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale));
  image_source->SetIcon(gfx::Image(image));
  // TODO(nullhook): Create badge and set text on badge via pref text
  gfx::ImageSkia icon(
      gfx::Image(gfx::ImageSkia(std::move(image_source), preferred_size))
          .AsImageSkia());
  // Use badge-and-icon source for button's image in all states
  SetImageModel(views::Button::STATE_NORMAL,
                ui::ImageModel::FromImageSkia(icon));

  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveShieldsActionViewHighlightPathGenerator>());
}

SkPath BraveShieldsActionView::GetHighlightPath() const {
  // Set the highlight path for the toolbar button,
  // making it inset so that the badge can show outside it in the
  // fake margin on the right that we are creating.
  gfx::Insets highlight_insets(0, 0, 0, kBraveActionRightMargin);
  gfx::Rect rect(GetPreferredSize());
  rect.Inset(highlight_insets);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kMaximum, rect.size());
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  return path;
}

void BraveShieldsActionView::ButtonPressed() {
  NOTIMPLEMENTED();
}

std::unique_ptr<views::LabelButtonBorder>
BraveShieldsActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets(0, 0, 0, 0));
  return border;
}

void BraveShieldsActionView::Update() {
  // We can get active webcontent's url and perform a GetBraveShieldsEnabled
  // check
  NOTIMPLEMENTED();
}
