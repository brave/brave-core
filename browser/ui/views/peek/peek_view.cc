/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_view.h"

#include <algorithm>
#include <string>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/browser/ui/views/peek/peek_shields_button.h"
#include "brave/browser/ui/views/view_shadow.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ssl/chrome_security_state_util.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_specification.h"
#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/omnibox/browser/location_bar_model_util.h"
#include "components/security_state/core/security_state.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/color/color_id.h"
#include "ui/color/color_provider.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/box_layout_view.h"
#include "ui/views/style/typography.h"
#include "ui/views/widget/widget.h"

namespace {

constexpr float kCardCornerRadius = 12.f;
constexpr int kToolbarHeight = 40;
constexpr int kToolbarHorizontalPadding = 12;
constexpr int kToolbarSpacing = 8;
constexpr int kFaviconSize = 16;
constexpr int kButtonIconSize = 16;
constexpr int kMinCardInset = 24;
// Fraction of the covered area the card leaves on each side.
constexpr float kCardInsetRatio = 0.075f;
constexpr float kOpenInitialScale = 0.9f;
constexpr base::TimeDelta kOpenDuration = base::Milliseconds(200);

constexpr ViewShadow::ShadowParameters kCardShadow{
    .offset_x = 0,
    .offset_y = 4,
    .blur_radius = 24,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 0.24 * 255)};

std::unique_ptr<views::ImageButton> CreateToolbarButton(
    views::Button::PressedCallback callback,
    const gfx::VectorIcon& icon,
    int message_id) {
  auto button = views::CreateVectorImageButtonWithNativeTheme(
      std::move(callback), icon, kButtonIconSize, nala::kColorIconDefault);
  const std::u16string text = l10n_util::GetStringUTF16(message_id);
  button->SetTooltipText(text);
  button->GetViewAccessibility().SetName(text);
  return button;
}

}  // namespace

PeekView::PeekView(Delegate& delegate, BrowserWindowInterface& browser)
    : AnimationDelegateViews(this), delegate_(delegate), browser_(browser) {
  open_animation_.SetSlideDuration(kOpenDuration);
  open_animation_.SetTweenType(gfx::Tween::EASE_OUT);
  open_animation_.Reset(1);

  GetViewAccessibility().SetRole(ax::mojom::Role::kDialog);
  GetViewAccessibility().SetName(
      l10n_util::GetStringUTF16(IDS_PEEK_ACCESSIBLE_NAME));
  AddAccelerator(ui::Accelerator(ui::VKEY_ESCAPE, ui::EF_NONE));

  card_ = AddChildView(std::make_unique<views::View>());
  card_->SetBackground(views::CreateRoundedRectBackground(
      nala::kColorContainerBackground, kCardCornerRadius));
  auto* card_layout =
      card_->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kVertical));
  card_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kStretch);

  auto* toolbar = card_->AddChildView(std::make_unique<views::BoxLayoutView>());
  toolbar->SetOrientation(views::BoxLayout::Orientation::kHorizontal);
  toolbar->SetInsideBorderInsets(gfx::Insets::VH(0, kToolbarHorizontalPadding));
  toolbar->SetBetweenChildSpacing(kToolbarSpacing);
  toolbar->SetCrossAxisAlignment(views::BoxLayout::CrossAxisAlignment::kCenter);
  toolbar->SetPreferredSize(gfx::Size(0, kToolbarHeight));

  site_info_button_ = toolbar->AddChildView(CreateToolbarButton(
      base::BindRepeating(&PeekView::ShowPageInfo, base::Unretained(this)),
      kLeoTuneSmallIcon, IDS_TOOLTIP_LOCATION_ICON));

  favicon_ = toolbar->AddChildView(std::make_unique<views::ImageView>());
  favicon_->SetImageSize(gfx::Size(kFaviconSize, kFaviconSize));

  title_ = toolbar->AddChildView(std::make_unique<views::Label>(
      std::u16string(), views::style::CONTEXT_LABEL,
      views::style::STYLE_BODY_3_MEDIUM));
  title_->SetEnabledColor(nala::kColorTextPrimary);
  title_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  title_->SetElideBehavior(gfx::ELIDE_TAIL);
  title_->SetAutoColorReadabilityEnabled(false);

  host_ = toolbar->AddChildView(std::make_unique<views::Label>(
      std::u16string(), views::style::CONTEXT_LABEL,
      views::style::STYLE_BODY_3));
  host_->SetEnabledColor(nala::kColorTextTertiary);
  host_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  host_->SetElideBehavior(gfx::ELIDE_HEAD);
  host_->SetAutoColorReadabilityEnabled(false);
  host_->SetDirectionalityMode(gfx::DirectionalityMode::DIRECTIONALITY_AS_URL);

  auto* spacer = toolbar->AddChildView(std::make_unique<views::View>());
  toolbar->SetFlexForView(spacer, 1);

  shields_button_ =
      toolbar->AddChildView(std::make_unique<PeekShieldsButton>(browser));

  open_as_tab_button_ = toolbar->AddChildView(CreateToolbarButton(
      base::BindRepeating(&Delegate::OnPeekViewOpenAsTabRequested,
                          base::Unretained(&delegate)),
      kLeoFullscreenOnIcon, IDS_PEEK_OPEN_AS_TAB));
  close_button_ = toolbar->AddChildView(CreateToolbarButton(
      base::BindRepeating(&Delegate::OnPeekViewCloseRequested,
                          base::Unretained(&delegate)),
      kLeoCloseIcon, IDS_PEEK_CLOSE));

  web_view_ = card_->AddChildView(
      std::make_unique<views::WebView>(browser.GetProfile()));
  card_layout->SetFlexForView(web_view_, 1);

  card_shadow_ = std::make_unique<ViewShadow>(
      card_, gfx::RoundedCornersF(kCardCornerRadius), kCardShadow);
}

PeekView::~PeekView() = default;

void PeekView::SetWebContents(content::WebContents* contents) {
  shields_button_->SetWebContents(contents);
  web_view_->SetWebContents(contents);
  if (contents) {
    web_view_->holder()->SetNativeViewCornerRadii(
        gfx::RoundedCornersF(0, 0, kCardCornerRadius, kCardCornerRadius));
  }
  UpdateToolbar();
}

void PeekView::UpdateToolbar() {
  content::WebContents* contents = web_view_->web_contents();
  if (!contents) {
    favicon_->SetImage(ui::ImageModel());
    title_->SetText(std::u16string());
    host_->SetText(std::u16string());
    return;
  }

  // Matches the omnibox, which uses the Nala icon for secure pages.
  const security_state::SecurityLevel security_level =
      chrome_security_state::GetSecurityLevel(contents);
  std::unique_ptr<security_state::VisibleSecurityState> security_state =
      chrome_security_state::GetVisibleSecurityState(contents);
  views::SetImageFromVectorIconWithColor(
      site_info_button_,
      security_level == security_state::SECURE
          ? kLeoTuneSmallIcon
          : location_bar_model::GetSecurityVectorIcon(security_level,
                                                      security_state.get()),
      kButtonIconSize,
      views::IconColors(nala::kColorIconDefault, nala::kColorIconDisabled));

  const GURL& url = contents->GetVisibleURL();
  title_->SetText(contents->GetTitle());
  host_->SetText(url_formatter::FormatUrlForSecurityDisplay(
      url, url_formatter::SchemeDisplay::OMIT_HTTP_AND_HTTPS));

  ui::ImageModel favicon;
  if (auto* driver = favicon::ContentFaviconDriver::FromWebContents(contents)) {
    if (gfx::Image image = driver->GetFavicon(); !image.IsEmpty()) {
      favicon = ui::ImageModel::FromImage(image);
    }
  }
  favicon_->SetImage(favicon);
  favicon_->SetVisible(!favicon.IsEmpty());
}

void PeekView::FocusContents() {
  web_view_->RequestFocus();
}

void PeekView::AnimateOpen() {
  if (!gfx::Animation::ShouldRenderRichAnimation()) {
    return;
  }
  open_animation_.Reset(0);
  open_animation_.Show();
}

void PeekView::ShowPageInfo() {
  content::WebContents* contents = web_view_->web_contents();
  if (!contents) {
    return;
  }
  content::NavigationEntry* entry = contents->GetController().GetVisibleEntry();
  if (!entry || entry->IsInitialEntry()) {
    return;
  }

  // The Peek contents isn't a tab, so page info can't look up its browser.
  std::unique_ptr<PageInfoBubbleSpecification> specification =
      PageInfoBubbleSpecification::Builder(
          views::BubbleAnchor(site_info_button_.get()),
          GetWidget()->GetNativeWindow(), contents, entry->GetVirtualURL())
          .AddGetBrowserCallback(base::BindRepeating(
              [](base::WeakPtr<BrowserWindowInterface> browser,
                 content::WebContents*) { return browser.get(); },
              browser_->GetWeakPtr()))
          .Build();
  views::BubbleDialogDelegateView* bubble =
      PageInfoBubbleView::CreatePageInfoBubble(std::move(specification));
  bubble->GetWidget()->Show();
}

void PeekView::Layout(PassKey) {
  const gfx::Rect bounds = GetLocalBounds();
  const int inset_x = std::max(
      kMinCardInset, static_cast<int>(bounds.width() * kCardInsetRatio));
  const int inset_y = std::max(
      kMinCardInset, static_cast<int>(bounds.height() * kCardInsetRatio));
  gfx::Rect card_bounds = bounds;
  card_bounds.Inset(gfx::Insets::VH(inset_y, inset_x));

  gfx::Rect initial_bounds = card_bounds;
  initial_bounds.ClampToCenteredSize(
      gfx::ScaleToRoundedSize(card_bounds.size(), kOpenInitialScale));
  card_->SetBoundsRect(gfx::Tween::RectValueBetween(
      open_animation_.GetCurrentValue(), initial_bounds, card_bounds));
}

void PeekView::OnPaintBackground(gfx::Canvas* canvas) {
  const SkColor scrim =
      GetColorProvider()->GetColor(nala::kColorDialogsScrimBackground);
  canvas->DrawColor(SkColorSetA(
      scrim, SkColorGetA(scrim) * open_animation_.GetCurrentValue()));
}

void PeekView::AnimationProgressed(const gfx::Animation* animation) {
  InvalidateLayout();
  SchedulePaint();
}

bool PeekView::OnMousePressed(const ui::MouseEvent& event) {
  if (!card_->bounds().Contains(event.location())) {
    delegate_->OnPeekViewCloseRequested();
  }
  return true;
}

bool PeekView::AcceleratorPressed(const ui::Accelerator& accelerator) {
  CHECK_EQ(accelerator.key_code(), ui::VKEY_ESCAPE);
  delegate_->OnPeekViewCloseRequested();
  return true;
}

BEGIN_METADATA(PeekView)
END_METADATA
