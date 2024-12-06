// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_actions/brave_shields_action_view.h"

#include <memory>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/brave_icon_with_badge_image_source.h"
#include "brave/browser/ui/webui/brave_shields/shields_panel_ui.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/color/color_provider_manager.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/image/image_skia_rep.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"
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

DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(BraveShieldsActionView,
                                      kShieldsActionIcon);

BraveShieldsActionView::BraveShieldsActionView(
    BrowserWindowInterface* browser_window_interface)
    : LabelButton(base::BindRepeating(&BraveShieldsActionView::ButtonPressed,
                                      base::Unretained(this),
                                      browser_window_interface),
                  std::u16string()),
      profile_(CHECK_DEREF(browser_window_interface->GetProfile())),
      tab_strip_model_(
          CHECK_DEREF(browser_window_interface->GetTabStripModel())) {
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  if (web_contents) {
    brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents)
        ->AddObserver(this);
  }

  SetAccessibleName(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_BRAVE_SHIELDS));
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetProperty(views::kElementIdentifierKey, kShieldsActionIcon);
  tab_strip_model_->AddObserver(this);

  // The MenuButtonController makes sure the panel closes when clicked if the
  // panel is already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&BraveShieldsActionView::ButtonPressed,
                          base::Unretained(this), browser_window_interface),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  menu_button_controller_ = menu_button_controller.get();
  SetButtonController(std::move(menu_button_controller));
}

BraveShieldsActionView::~BraveShieldsActionView() {
  auto* web_contents = tab_strip_model_->GetActiveWebContents();
  if (web_contents) {
    brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents)
        ->RemoveObserver(this);
  }
}

void BraveShieldsActionView::Init() {
  UpdateIconState();
  views::HighlightPathGenerator::Install(
      this, std::make_unique<BraveShieldsActionViewHighlightPathGenerator>());
}

SkPath BraveShieldsActionView::GetHighlightPath() const {
  // Set the highlight path for the toolbar button,
  // making it inset so that the badge can show outside it in the
  // fake margin on the right that we are creating.
  auto highlight_insets =
      gfx::Insets::TLBR(0, 0, 0, -1 * kBraveActionLeftMarginExtra);
  gfx::Rect rect(GetPreferredSize());
  rect.Inset(highlight_insets);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kHigh, rect.size());
  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  return path;
}

std::unique_ptr<IconWithBadgeImageSource>
BraveShieldsActionView::GetImageSource() {
  auto preferred_size = GetPreferredSize();
  auto* web_contents = tab_strip_model_->GetActiveWebContents();

  auto get_color_provider_callback = base::BindRepeating(
      [](base::WeakPtr<content::WebContents> weak_web_contents) {
        const auto* const color_provider =
            weak_web_contents
                ? &weak_web_contents->GetColorProvider()
                : ui::ColorProviderManager::Get().GetColorProviderFor(
                      ui::NativeTheme::GetInstanceForNativeUi()
                          ->GetColorProviderKey(nullptr));
        return color_provider;
      },
      web_contents ? web_contents->GetWeakPtr()
                   : base::WeakPtr<content::WebContents>());

  std::unique_ptr<IconWithBadgeImageSource> image_source(
      new brave::BraveIconWithBadgeImageSource(
          preferred_size, std::move(get_color_provider_callback),
          GetLayoutConstant(LOCATION_BAR_TRAILING_ICON_SIZE),
          kBraveActionLeftMarginExtra));
  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  bool is_enabled = false;
  std::string badge_text;

  if (web_contents) {
    auto* shields_data_controller =
        brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);

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

  if (is_enabled &&
      profile_->GetPrefs()->GetBoolean(kShieldsStatsBadgeVisible)) {
    image_source->SetBadge(std::move(badge));
  }

  return image_source;
}

gfx::ImageSkia BraveShieldsActionView::GetIconImage(bool is_enabled) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia image;
  const SkBitmap bitmap =
      rb.GetImageNamed(is_enabled ? IDR_BRAVE_SHIELDS_ICON_64
                                  : IDR_BRAVE_SHIELDS_ICON_64_DISABLED)
          .AsBitmap();
  float scale = static_cast<float>(bitmap.width()) /
                GetLayoutConstant(LOCATION_BAR_TRAILING_ICON_SIZE);
  image.AddRepresentation(gfx::ImageSkiaRep(bitmap, scale));
  return image;
}

void BraveShieldsActionView::UpdateIconState() {
  const gfx::ImageSkia icon(GetImageSource(), GetPreferredSize());
  // Use badge-and-icon source for button's image in all states
  SetImageModel(views::Button::STATE_NORMAL,
                ui::ImageModel::FromImageSkia(icon));
}

void BraveShieldsActionView::ButtonPressed(
    BrowserWindowInterface* browser_window_interface) {
  auto* web_content = tab_strip_model_->GetActiveWebContents();
  if (web_content && SchemeIsLocal(web_content->GetLastCommittedURL())) {
    return;  // Do not show bubble if it's a local scheme
  }

  if (!webui_bubble_manager_) {
    webui_bubble_manager_ = WebUIBubbleManager::Create<ShieldsPanelUI>(
        this, browser_window_interface, GURL(kShieldsPanelURL),
        IDS_BRAVE_SHIELDS);
  }

  if (webui_bubble_manager_->GetBubbleWidget()) {
    webui_bubble_manager_->CloseBubble();
    return;
  }

  webui_bubble_manager_->ShowBubble();
}

bool BraveShieldsActionView::SchemeIsLocal(GURL url) {
  return url.SchemeIs(url::kAboutScheme) || url.SchemeIs(url::kBlobScheme) ||
         url.SchemeIs(url::kDataScheme) ||
         url.SchemeIs(url::kFileSystemScheme) || url.SchemeIs(kMagnetScheme) ||
         url.SchemeIs(kBraveUIScheme) || url.SchemeIs(content::kChromeUIScheme);
}

std::unique_ptr<views::LabelButtonBorder>
BraveShieldsActionView::CreateDefaultBorder() const {
  std::unique_ptr<views::LabelButtonBorder> border =
      LabelButton::CreateDefaultBorder();
  border->set_insets(gfx::Insets::TLBR(0, 0, 0, 0));
  return border;
}

std::u16string BraveShieldsActionView::GetTooltipText(
    const gfx::Point& p) const {
  auto* web_contents = tab_strip_model_->GetActiveWebContents();

  if (web_contents) {
    auto* shields_data_controller =
        brave_shields::BraveShieldsTabHelper::FromWebContents(web_contents);

    int count = shields_data_controller->GetTotalBlockedCount();

    if (count > 0) {
      return l10n_util::GetStringFUTF16Int(IDS_BRAVE_SHIELDS_ICON_TOOLTIP,
                                           count);
    }
  }

  return brave_l10n::GetLocalizedResourceUTF16String(IDS_BRAVE_SHIELDS);
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

void BraveShieldsActionView::Update() {
  UpdateIconState();
}

void BraveShieldsActionView::OnResourcesChanged() {
  UpdateIconState();
}

void BraveShieldsActionView::OnShieldsEnabledChanged() {
  UpdateIconState();
}

void BraveShieldsActionView::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (selection.active_tab_changed()) {
    if (selection.new_contents) {
      brave_shields::BraveShieldsTabHelper::FromWebContents(
          selection.new_contents)
          ->AddObserver(this);
    }

    if (selection.old_contents) {
      brave_shields::BraveShieldsTabHelper::FromWebContents(
          selection.old_contents)
          ->RemoveObserver(this);
    }
    UpdateIconState();
  }
}

BEGIN_METADATA(BraveShieldsActionView)
END_METADATA
