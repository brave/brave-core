/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_action/wayback_machine_action_icon_view.h"

#include <memory>

#include "base/notreached.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/page_action/page_action_icon_loading_indicator_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"

namespace {

// For customizing badge position.
class WaybackIconImageSource : public gfx::CanvasImageSource {
 public:
  WaybackIconImageSource(const gfx::IconDescription& icon,
                         const gfx::IconDescription& badge)
      : CanvasImageSource(gfx::Size(icon.dip_size, icon.dip_size)),
        icon_description_(icon),
        badge_description_(badge) {}
  ~WaybackIconImageSource() override = default;

  // gfx::CanvasImageSource overrides:
  void Draw(gfx::Canvas* canvas) override {
    const gfx::ImageSkia icon_image = gfx::CreateVectorIcon(icon_description_);
    const gfx::ImageSkia badge_image =
        gfx::CreateVectorIcon(badge_description_);
    canvas->DrawImageInt(icon_image, 0, 0);

    // Assume that icon & badge are both square and locate badge on the bottom
    // right of the icon.
    const int badge_position =
        icon_description_.dip_size - badge_description_.dip_size;
    canvas->DrawImageInt(badge_image, badge_position, badge_position);
  }

 private:
  const gfx::IconDescription icon_description_;
  const gfx::IconDescription badge_description_;
};

// Customize upstream loading indicator to have different indicator size.
class WaybackLoadingIndicatorView : public PageActionIconLoadingIndicatorView {
  METADATA_HEADER(WaybackLoadingIndicatorView,
                  PageActionIconLoadingIndicatorView)

 public:
  using PageActionIconLoadingIndicatorView::PageActionIconLoadingIndicatorView;
  ~WaybackLoadingIndicatorView() override = default;

  // PageActionIconLoadingIndicatorView overrides:
  void OnViewBoundsChanged(views::View* observed_view) override {
    const auto bounds = observed_view->GetLocalBounds();
    constexpr int kIndicatorSize = 24;
    auto center_point = bounds.CenterPoint();
    center_point.Offset(-(kIndicatorSize / 2), -(kIndicatorSize / 2));
    gfx::Rect indicator_bounds(center_point, {kIndicatorSize, kIndicatorSize});
    SetBoundsRect(indicator_bounds);
  }
};

BEGIN_METADATA(WaybackLoadingIndicatorView);
END_METADATA

}  // namespace

WaybackMachineActionIconView::WaybackMachineActionIconView(
    CommandUpdater* command_updater,
    Browser* browser,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : PageActionIconView(command_updater,
                         IDC_SHOW_WAYBACK_MACHINE_BUBBLE,
                         icon_label_bubble_delegate,
                         page_action_icon_delegate,
                         "WaybackMachineActionIconView",
                         false),
      state_manager_(this, browser),
      browser_(*browser) {
  SetLoadingIndicator(std::make_unique<WaybackLoadingIndicatorView>(this));
}

WaybackMachineActionIconView::~WaybackMachineActionIconView() = default;

views::BubbleDialogDelegate* WaybackMachineActionIconView::GetBubble() const {
  if (!GetWebContents()) {
    return nullptr;
  }

  // tab helper knows current bubble window.
  auto* tab_helper =
      BraveWaybackMachineTabHelper::FromWebContents(GetWebContents());
  if (!tab_helper) {
    return nullptr;
  }

  auto* widget =
      views::Widget::GetWidgetForNativeWindow(tab_helper->active_window());
  if (!widget) {
    return nullptr;
  }

  return static_cast<views::BubbleDialogDelegate*>(widget->widget_delegate());
}

const gfx::VectorIcon& WaybackMachineActionIconView::GetVectorIcon() const {
  return kLeoHistoryIcon;
}

ui::ImageModel WaybackMachineActionIconView::GetSizedIconImage(int size) const {
  const auto state = state_manager_.GetActiveTabWaybackState();
  if (state != WaybackState::kLoaded && state != WaybackState::kNotAvailable) {
    return PageActionIconView::GetSizedIconImage(size);
  }

  constexpr int kBadgeSize = 8;
  const SkColor icon_color =
      GetActive() ? views::GetCascadingAccentColor(
                        const_cast<WaybackMachineActionIconView*>(this))
                  : GetIconColor();
  gfx::IconDescription icon_description(kLeoHistoryIcon, size, icon_color);
  auto* cp = GetColorProvider();
  if (state == WaybackState::kLoaded) {
    const SkColor badge_color = cp ? cp->GetColor(kColorWaybackMachineURLLoaded)
                                   : gfx::kPlaceholderColor;
    gfx::IconDescription badge_description(kLeoDesktopVpnOnColorIcon,
                                           kBadgeSize, badge_color);
    gfx::ImageSkia icon_image(
        std::make_unique<WaybackIconImageSource>(icon_description,
                                                 badge_description),
        gfx::Size(icon_description.dip_size, icon_description.dip_size));
    return ui::ImageModel::FromImageSkia(icon_image);
  }

  const SkColor badge_color =
      cp ? cp->GetColor(kColorWaybackMachineURLNotAvailable)
         : gfx::kPlaceholderColor;
  gfx::IconDescription badge_description(kLeoDesktopVpnErrorColorIcon,
                                         kBadgeSize, badge_color);
  gfx::ImageSkia icon_image(
      std::make_unique<WaybackIconImageSource>(icon_description,
                                               badge_description),
      gfx::Size(icon_description.dip_size, icon_description.dip_size));
  return ui::ImageModel::FromImageSkia(icon_image);
}

void WaybackMachineActionIconView::OnExecuting(ExecuteSource source) {
  // If user clicks this icon when state is kFetching or kLoaded, we don't
  // launch the bubble. But highlight is set but not cleared as bubble is
  // not launched. To prevent this, clear the highlight and let bubble control
  // its anchor's highlight.
  SetHighlighted(false);
}

void WaybackMachineActionIconView::UpdateImpl() {
  const auto state = state_manager_.GetActiveTabWaybackState();
  switch (state) {
    case WaybackState::kInitial:
      SetVisible(false);
      SetIsLoading(false);
      SetCommandEnabled(false);
      break;
    case WaybackState::kNeedToCheck:
      SetVisible(true);
      SetIsLoading(false);
      SetCommandEnabled(true);
      break;
    case WaybackState::kFetching:
      SetVisible(true);
      SetIsLoading(true);
      SetCommandEnabled(false);
      break;
    case WaybackState::kLoaded:
      SetVisible(true);
      SetIsLoading(false);
      SetCommandEnabled(false);
      break;
    case WaybackState::kNotAvailable:
      SetVisible(true);
      SetIsLoading(false);
      SetCommandEnabled(true);
      break;
    default:
      NOTREACHED() << "All states are handled above";
  }

  // We should update icon also as each state could has different colored badge
  // icon.
  UpdateIconImage();
}

void WaybackMachineActionIconView::ExecuteCommandForTesting() {
  PageActionIconView::ExecuteCommand(PageActionIconView::EXECUTE_SOURCE_MOUSE);
}

BEGIN_METADATA(WaybackMachineActionIconView);
END_METADATA
