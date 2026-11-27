/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/overlay/brave_video_overlay_window_views.h"

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <utility>

#include "base/supports_user_data.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/views/overlay/back_to_tab_button.h"
#include "chrome/browser/ui/views/overlay/back_to_tab_label_button.h"
#include "chrome/browser/ui/views/overlay/close_image_button.h"
#include "chrome/browser/ui/views/overlay/constants.h"
#include "chrome/browser/ui/views/overlay/hang_up_button.h"
#include "chrome/browser/ui/views/overlay/minimize_button.h"
#include "chrome/browser/ui/views/overlay/playback_image_button.h"
#include "chrome/browser/ui/views/overlay/simple_overlay_window_image_button.h"
#include "chrome/browser/ui/views/overlay/toggle_camera_button.h"
#include "chrome/browser/ui/views/overlay/toggle_microphone_button.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/picture_in_picture_window_controller.h"
#include "content/public/browser/web_contents.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/resize_utils.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/view_utils.h"

#if BUILDFLAG(IS_LINUX)
#include "chrome/browser/shell_integration_linux.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_native_widget_aura_linux.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_widget.h"
#include "components/tabs/public/tab_interface.h"
#endif  // BUILDFLAG(IS_LINUX)

namespace {

constexpr int kTopControlIconSize = 20;

constexpr char kSavedPipWindowBoundsKey[] = "BraveSavedPipWindowBounds";

// Most recent PiP window bounds for a BrowserContext, present only once bounds
// have been saved. Every profile, including each off-the-record one, has its
// own copy, so private windows never share bounds with regular ones and their
// bounds go away with the private session. Nothing is persisted to disk.
struct SavedPipWindowBounds : public base::SupportsUserData::Data {
  // Display the window was on, and its bounds relative to the origin of that
  // display's work area.
  int64_t display_id = display::kInvalidDisplayId;
  gfx::Rect bounds_in_work_area;
  // Used to find the closest display when `display_id` no longer exists.
  gfx::Rect bounds_in_screen;
};

content::BrowserContext* GetBrowserContext(
    content::PictureInPictureWindowController* controller) {
  if (!controller || !controller->GetWebContents()) {
    return nullptr;
  }
  return controller->GetWebContents()->GetBrowserContext();
}

SavedPipWindowBounds* GetSavedPipWindowBounds(
    content::BrowserContext* context) {
  return static_cast<SavedPipWindowBounds*>(
      context->GetUserData(kSavedPipWindowBoundsKey));
}

// Picks the edge to resize from so that the corner closest to the edge of the
// work area stays put, matching upstream's quadrant-based behavior.
gfx::ResizeEdge GetResizeEdge(const gfx::Rect& bounds,
                              const gfx::Rect& work_area) {
  const gfx::Point center = bounds.CenterPoint();
  const gfx::Point work_area_center = work_area.CenterPoint();
  const bool top = center.y() < work_area_center.y();
  if (center.x() < work_area_center.x()) {
    return top ? gfx::ResizeEdge::kBottomRight : gfx::ResizeEdge::kTopRight;
  }
  return top ? gfx::ResizeEdge::kBottomLeft : gfx::ResizeEdge::kTopLeft;
}

}  // namespace

BraveVideoOverlayWindowViews::BraveVideoOverlayWindowViews(
    content::VideoPictureInPictureWindowController* controller)
    : VideoOverlayWindowViews(controller) {}

BraveVideoOverlayWindowViews::~BraveVideoOverlayWindowViews() = default;

#if BUILDFLAG(IS_LINUX)
// Sets WM_CLASS (X11) and the app id (Wayland) so Linux window managers can
// match the window to the browser's (or the source web app's) .desktop entry.
// Upstream leaves these unset, so the window ends up with neither.
void BraveVideoOverlayWindowViews::Init(views::Widget::InitParams params) {
  params.wm_class_name = shell_integration_linux::GetProgramClassName();
  params.wm_class_class = shell_integration_linux::GetProgramClassClass();
  params.wayland_app_id = params.wm_class_class;

  // A video from a browser window takes that window's identity: its web app's,
  // or its profile's desktop entry, like the Windows taskbar-id handling in
  // VideoOverlayWindowViews::Create().
  auto* tab = tabs::TabInterface::MaybeGetFromContents(
      GetController()->GetWebContents());
  if (auto* browser = tab ? tab->GetBrowserWindowInterface() : nullptr) {
    const BrowserNativeWidgetAuraLinux::WindowClass window_class =
        static_cast<BrowserNativeWidgetAuraLinux*>(
            BrowserView::GetBrowserViewForBrowser(browser)
                ->browser_widget()
                ->browser_native_widget())
            ->GetWindowClass();
    params.wm_class_name = window_class.wm_class_name;
    params.wm_class_class = window_class.wm_class_class;
    params.wayland_app_id = window_class.wayland_app_id;
  }

  views::Widget::Init(std::move(params));
}
#endif  // BUILDFLAG(IS_LINUX)

void BraveVideoOverlayWindowViews::ShowInactive() {
  MaybeRestoreCachedBounds();
  VideoOverlayWindowViews::ShowInactive();
}

void BraveVideoOverlayWindowViews::Hide() {
  MaybeCacheBounds();
  VideoOverlayWindowViews::Hide();
}

void BraveVideoOverlayWindowViews::Close() {
  // Called when the tab is closed while the window is still showing.
  MaybeCacheBounds();
  VideoOverlayWindowViews::Close();
}

void BraveVideoOverlayWindowViews::OnNativeWidgetDestroying() {
  // Covers the window being closed by the OS rather than by the controller.
  MaybeCacheBounds();
  VideoOverlayWindowViews::OnNativeWidgetDestroying();
}

void BraveVideoOverlayWindowViews::SetUpViews() {
  VideoOverlayWindowViews::SetUpViews();
  UpdateControlIcons();
}

void BraveVideoOverlayWindowViews::OnUpdateControlsBounds() {
  VideoOverlayWindowViews::OnUpdateControlsBounds();

  LayoutTopControls();
  LayoutCenterControls();
}

void BraveVideoOverlayWindowViews::MaybeCacheBounds() {
  // Bounds set while the window is hidden are placeholders computed before it
  // is shown, and tucked bounds are mostly off-screen on purpose.
  if (!IsVisible() || is_tucking_forced_) {
    return;
  }

  const gfx::Rect bounds = GetBounds();
  if (bounds.IsEmpty()) {
    return;
  }

  content::BrowserContext* context = GetBrowserContext(GetController());
  if (!context) {
    return;
  }

  const display::Display display =
      display::Screen::Get()->GetDisplayNearestWindow(GetNativeWindow());
  auto saved = std::make_unique<SavedPipWindowBounds>();
  saved->display_id = display.id();
  saved->bounds_in_work_area = bounds - display.work_area().OffsetFromOrigin();
  saved->bounds_in_screen = bounds;
  context->SetUserData(kSavedPipWindowBoundsKey, std::move(saved));
}

void BraveVideoOverlayWindowViews::MaybeRestoreCachedBounds() {
  if (IsVisible()) {
    return;
  }

  content::BrowserContext* context = GetBrowserContext(GetController());
  const SavedPipWindowBounds* saved =
      context ? GetSavedPipWindowBounds(context) : nullptr;
  if (!saved) {
    return;
  }

  // Restore relative to the display the window was last on, so it stays there
  // even if the display layout changed. If that display is gone, fall back to
  // the display closest to the old screen bounds.
  const display::Screen* screen = display::Screen::Get();
  display::Display display;
  gfx::Rect bounds;
  if (screen->GetDisplayWithDisplayId(saved->display_id, &display)) {
    bounds =
        saved->bounds_in_work_area + display.work_area().OffsetFromOrigin();
  } else {
    display = screen->GetDisplayMatching(saved->bounds_in_screen);
    bounds = saved->bounds_in_screen;
  }

  const gfx::Rect work_area = display.work_area();
  if (work_area.IsEmpty()) {
    return;
  }

  UpdateMaxSize(work_area);
  gfx::Size size = bounds.size();
  size.SetToMin(max_size_);
  size.SetToMax(GetMinimumSize());
  bounds.set_size(size);

  // The new video can have a different aspect ratio than the previous one.
  if (!natural_size_.IsEmpty()) {
    const float aspect_ratio =
        static_cast<float>(natural_size_.width()) / natural_size_.height();
    gfx::SizeRectToAspectRatio(GetResizeEdge(bounds, work_area), aspect_ratio,
                               GetMinimumSize(), max_size_, &bounds);
  }

  bounds.AdjustToFit(work_area);
  SetBounds(bounds);

  // Makes later bounds updates, e.g. from UpdateNaturalSize(), keep the
  // restored size and position instead of resetting to the defaults.
  has_been_shown_ = true;
}

void BraveVideoOverlayWindowViews::UpdateControlIcons() {
  close_controls_view_->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoCloseIcon, kColorPipWindowForeground,
                                     kTopControlIconSize));

  if (minimize_button_) {
    minimize_button_->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(kChromiumMinimizeOldIcon,
                                       kColorPipWindowForeground,
                                       kTopControlIconSize));
  }

  previous_track_controls_view_->override_icon(kLeoPreviousOutlineIcon);
  next_track_controls_view_->override_icon(kLeoNextOutlineIcon);
}

void BraveVideoOverlayWindowViews::LayoutTopControls() {
  // Copies size in order to avoid ASAN failure
  // https://github.com/brave/internal/issues/1108
  const auto window_size = GetBounds().size();

  // Lay out controls on top of window
  constexpr int kTopControlSpacing = 16;
  auto get_new_size = [](const gfx::Insets& insets) {
    return gfx::Size(kTopControlIconSize + insets.width(),
                     kTopControlIconSize + insets.height());
  };

  // Lay out controls from right to left. As we'd like to apply spacing based
  // on the "Icon" size, we take away insets when calculating spacing.
  views::View* previous_view = nullptr;
  for (auto* view : std::initializer_list<views::View*>{
           close_controls_view_.get(), back_to_tab_label_button_.get(),
           minimize_button_.get()}) {
    if (!view || view->GetVisible()) {
      continue;
    }

    auto insets = view->GetInsets();
    close_controls_view_->SetSize(get_new_size(insets));

    const int x =
        previous_view
            ? previous_view->origin().x() -
                  (kTopControlSpacing - previous_view->GetInsets().left() -
                   insets.right()) -
                  view->size().width()
            : window_size.width() - (kTopControlSpacing - insets.right()) -
                  view->size().width();
    const int y = kTopControlSpacing - insets.top();
    // Note that we're usin View::SetPosition() here instead of
    // CloseImageButton::SetPosition() and BackToTabButton::SetPosition().
    view->SetPosition({x, y});

    previous_view = view;
  }
}

void BraveVideoOverlayWindowViews::LayoutCenterControls() {
  // Lay out controls in the middle of window
  const auto window_size = GetBounds().size();

  std::vector<OverlayWindowImageButton*> visible_controls;
  if (replay_10_seconds_button_->GetVisible()) {
    visible_controls.push_back(replay_10_seconds_button_.get());
  }
  if (play_pause_controls_view_->GetVisible()) {
    visible_controls.push_back(play_pause_controls_view_.get());
  }
  if (forward_10_seconds_button_->GetVisible()) {
    visible_controls.push_back(forward_10_seconds_button_.get());
  }
  if (toggle_microphone_button_->GetVisible()) {
    visible_controls.push_back(toggle_microphone_button_.get());
  }
  if (toggle_camera_button_->GetVisible()) {
    visible_controls.push_back(toggle_camera_button_.get());
  }
  if (hang_up_button_->GetVisible()) {
    visible_controls.push_back(hang_up_button_.get());
  }

  constexpr int kCenterControlIconSize = 32;
  constexpr int kCenterControlSize =
      kCenterControlIconSize + kPipWindowIconPadding * 2;
  constexpr int kCenterControlSpacing = 24 - kPipWindowIconPadding * 2;

  const auto visible_controls_size = visible_controls.size();
  int x = (window_size.width() - visible_controls_size * kCenterControlSize -
           kCenterControlSpacing * (visible_controls_size - 1)) /
          2;
  const int y = (window_size.height() - kCenterControlSize) / 2;
  for (size_t i = 0; i < visible_controls_size; i++) {
    if (views::IsViewClass<SimpleOverlayWindowImageButton>(
            visible_controls[i])) {
      views::AsViewClass<SimpleOverlayWindowImageButton>(visible_controls[i])
          ->set_icon_size(kCenterControlIconSize);
    }
    visible_controls[i]->SetBounds(x, y, kCenterControlSize,
                                   kCenterControlSize);
    x += kCenterControlSize + kCenterControlSpacing;
  }
}

void BraveVideoOverlayWindowViews::OnKeyEvent(ui::KeyEvent* event) {
  if (event->type() == ui::EventType::kKeyPressed) {
    if (event->key_code() == ui::VKEY_LEFT) {
      Replay10Seconds();
      event->SetHandled();
      return;
    }
    if (event->key_code() == ui::VKEY_RIGHT) {
      Forward10Seconds();
      event->SetHandled();
      return;
    }
  }

  VideoOverlayWindowViews::OnKeyEvent(event);
}
