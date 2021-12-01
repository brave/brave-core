// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/ui/brave_actions/brave_action_icon_with_badge_image_source.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
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
#include "url/gurl.h"

namespace {

constexpr SkColor kBadgeBg = SkColorSetRGB(0x63, 0x64, 0x72);
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

BraveShieldsActionView::BraveShieldsActionView(Profile* profile,
                                               TabStripModel* tab_strip_model)
    : LabelButton(base::BindRepeating(&BraveShieldsActionView::ButtonPressed,
                                      base::Unretained(this)),
                  std::u16string()) {
  profile_ = profile;
  tab_strip_model_ = tab_strip_model;
  auto* ink_drop = views::InkDrop::Get(this);
  ink_drop->SetMode(views::InkDropHost::InkDropMode::ON);
  ink_drop->SetBaseColorCallback(base::BindRepeating(
      [](views::View* host) { return GetToolbarInkDropBaseColor(host); },
      this));

  SetAccessibleName(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  SetHasInkDropActionOnClick(true);
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  ink_drop->SetVisibleOpacity(kToolbarInkDropVisibleOpacity);
  tab_strip_model_->AddObserver(this);
}

BraveShieldsActionView::~BraveShieldsActionView() = default;

void BraveShieldsActionView::Init() {
  UpdateIconState();
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

std::unique_ptr<IconWithBadgeImageSource>
BraveShieldsActionView::GetImageSource() {
  auto preferred_size = GetPreferredSize();
  std::unique_ptr<IconWithBadgeImageSource> image_source(
      new BraveActionIconWithBadgeImageSource(preferred_size));
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  bool is_enabled = false;
  std::string badge_text;

  if (web_contents) {
    auto* shields_data_controller =
        brave_shields::BraveShieldsDataController::FromWebContents(
            web_contents);
    int count = shields_data_controller->GetTotalBlockedCount();

    if (count > 0) {
      badge_text = count > 99 ? "99+" : std::to_string(count);
    }

    is_enabled = shields_data_controller->GetBraveShieldsEnabled();

    if (!badge_text.empty()) {
      badge = std::make_unique<IconWithBadgeImageSource::Badge>(
          badge_text, SK_ColorWHITE, kBadgeBg);
    }
  }

  image_source->SetIcon(gfx::Image(GetIconImage(is_enabled)));
  if (is_enabled)
    image_source->SetBadge(std::move(badge));

  return image_source;
}

gfx::ImageSkia BraveShieldsActionView::GetIconImage(bool is_enabled) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia image;
  const SkBitmap bitmap =
      rb.GetImageNamed(is_enabled ? IDR_BRAVE_SHIELDS_ICON_64
                                  : IDR_BRAVE_SHIELDS_ICON_64_DISABLED)
          .AsBitmap();
  float scale = static_cast<float>(bitmap.width()) / kBraveActionGraphicSize;
  image.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale));
  return image;
}

void BraveShieldsActionView::UpdateIconState() {
  const gfx::ImageSkia icon(GetImageSource(), GetPreferredSize());
  // Use badge-and-icon source for button's image in all states
  SetImageModel(views::Button::STATE_NORMAL,
                ui::ImageModel::FromImageSkia(icon));
}

void BraveShieldsActionView::ButtonPressed() {
  if (!webui_bubble_manager_) {
    webui_bubble_manager_ =
        std::make_unique<WebUIBubbleManagerT<ShieldsPanelUI>>(
            this, profile_, GURL(kShieldsPanelURL), 1);
  }

  if (webui_bubble_manager_->GetBubbleWidget()) {
    webui_bubble_manager_->CloseBubble();
    return;
  }

  webui_bubble_manager_->ShowBubble();
}

std::unique_ptr<views::LabelButtonBorder>
BraveShieldsActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets(0, 0, 0, 0));
  return border;
}

void BraveShieldsActionView::Update() {
  UpdateIconState();
}

void BraveShieldsActionView::OnResourcesChanged() {
  UpdateIconState();
}

void BraveShieldsActionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    if (selection.new_contents) {
      brave_shields::BraveShieldsDataController::FromWebContents(
          selection.new_contents)
          ->AddObserver(this);
    }

    if (selection.old_contents) {
      brave_shields::BraveShieldsDataController::FromWebContents(
          selection.old_contents)
          ->RemoveObserver(this);
    }
  }
}
