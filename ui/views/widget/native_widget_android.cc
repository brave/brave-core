// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/views/widget/native_widget_android.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "ui/base/class_property.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/ui_base_types.h"
#include "ui/compositor/layer.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/native_theme/native_theme_android.h"
#include "ui/views/drag_utils.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/drop_helper.h"
#include "ui/views/widget/focus_manager_event_handler.h"
#include "ui/views/widget/native_widget_delegate.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/tooltip_manager_aura.h"
#include "ui/views/widget/widget_aura_utils.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/window_reorderer.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/transient_window_manager.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_properties.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/activation_client.h"
#include "ui/wm/public/window_move_client.h"

#if defined(OS_WIN)
#include "base/win/scoped_gdi_object.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_win.h"
#endif

#if defined(USE_X11)
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"
#endif

#if !defined(OS_CHROMEOS)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#endif

DEFINE_UI_CLASS_PROPERTY_TYPE(views::internal::NativeWidgetPrivate*)

namespace views {

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetAndroid, public:

NativeWidgetAndroid::NativeWidgetAndroid(internal::NativeWidgetDelegate* delegate)
    : delegate_(delegate),
      ownership_(Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET),
      destroying_(false) {
}

// static
void NativeWidgetAndroid::RegisterNativeWidgetForWindow(
      internal::NativeWidgetPrivate* native_widget,
      gfx::NativeWindow window) {
}

// static
void NativeWidgetAndroid::AssignIconToAuraWindow(gfx::NativeWindow window,
                                              const gfx::ImageSkia& window_icon,
                                              const gfx::ImageSkia& app_icon) {
}

// static
void NativeWidgetAndroid::SetShadowElevationFromInitParams(
    gfx::NativeWindow window,
    const Widget::InitParams& params) {
}

// static
void NativeWidgetAndroid::SetResizeBehaviorFromDelegate(WidgetDelegate* delegate,
                                                     gfx::NativeWindow window) {
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetAndroid, internal::NativeWidgetPrivate implementation:

void NativeWidgetAndroid::InitNativeWidget(Widget::InitParams params) {
  delegate_->OnNativeWidgetCreated();
}

void NativeWidgetAndroid::OnWidgetInitDone() {}

NonClientFrameView* NativeWidgetAndroid::CreateNonClientFrameView() {
  return nullptr;
}

bool NativeWidgetAndroid::ShouldUseNativeFrame() const {
  // There is only one frame type for aura.
  return false;
}

bool NativeWidgetAndroid::ShouldWindowContentsBeTransparent() const {
  return false;
}

void NativeWidgetAndroid::FrameTypeChanged() {
  // This is called when the Theme has changed; forward the event to the root
  // widget.
  GetWidget()->ThemeChanged();
  GetWidget()->GetRootView()->SchedulePaint();
}

const Widget* NativeWidgetAndroid::GetWidgetImpl() const {
  return delegate_->AsWidget();	
}

Widget* NativeWidgetAndroid::GetWidget() {
  return delegate_->AsWidget();
}

const Widget* NativeWidgetAndroid::GetWidget() const {
  return delegate_->AsWidget();
}

gfx::NativeView NativeWidgetAndroid::GetNativeView() const {
  return window_;
}

gfx::NativeWindow NativeWidgetAndroid::GetNativeWindow() const {
  return window_;
}

Widget* NativeWidgetAndroid::GetTopLevelWidget() {
  NativeWidgetPrivate* native_widget = GetTopLevelNativeWidget(GetNativeView());
  return native_widget ? native_widget->GetWidget() : nullptr;
}

const ui::Compositor* NativeWidgetAndroid::GetCompositor() const {
  return nullptr;
}

const ui::Layer* NativeWidgetAndroid::GetLayer() const {
  return nullptr;
}

void NativeWidgetAndroid::ReorderNativeViews() {
}

void NativeWidgetAndroid::ViewRemoved(View* view) {
  DCHECK(drop_helper_.get() != nullptr);
  drop_helper_->ResetTargetViewIfEquals(view);
}

void NativeWidgetAndroid::SetNativeWindowProperty(const char* name, void* value) {
}

void* NativeWidgetAndroid::GetNativeWindowProperty(const char* name) const {
  return nullptr;
}

TooltipManager* NativeWidgetAndroid::GetTooltipManager() const {
  return tooltip_manager_.get();
}

void NativeWidgetAndroid::SetCapture() {
}

void NativeWidgetAndroid::ReleaseCapture() {
}

bool NativeWidgetAndroid::HasCapture() const {
  return false;
}

ui::InputMethod* NativeWidgetAndroid::GetInputMethod() {
  return nullptr;
}

void NativeWidgetAndroid::CenterWindow(const gfx::Size& size) {
}

void NativeWidgetAndroid::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
}

bool NativeWidgetAndroid::SetWindowTitle(const base::string16& title) {
  return true;
}

void NativeWidgetAndroid::SetWindowIcons(const gfx::ImageSkia& window_icon,
                                      const gfx::ImageSkia& app_icon) {
}

void NativeWidgetAndroid::InitModalType(ui::ModalType modal_type) {
}

gfx::Rect NativeWidgetAndroid::GetWindowBoundsInScreen() const {
  return gfx::Rect();
}

gfx::Rect NativeWidgetAndroid::GetClientAreaBoundsInScreen() const {
  return gfx::Rect();
}

gfx::Rect NativeWidgetAndroid::GetRestoredBounds() const {
  return gfx::Rect();
}

std::string NativeWidgetAndroid::GetWorkspace() const {
  return std::string();
}

void NativeWidgetAndroid::SetBounds(const gfx::Rect& bounds) {
}

void NativeWidgetAndroid::SetBoundsConstrained(const gfx::Rect& bounds) {
}

void NativeWidgetAndroid::SetSize(const gfx::Size& size) {
}

void NativeWidgetAndroid::StackAbove(gfx::NativeView native_view) {
}

void NativeWidgetAndroid::StackAtTop() {
}

void NativeWidgetAndroid::SetShape(std::unique_ptr<Widget::ShapeRects> shape) {
}

void NativeWidgetAndroid::Close() {
}

void NativeWidgetAndroid::CloseNow() {
}

void NativeWidgetAndroid::Show(ui::WindowShowState show_state,
                            const gfx::Rect& restore_bounds) {
}

void NativeWidgetAndroid::Hide() {
}

bool NativeWidgetAndroid::IsVisible() const {
  return true;
}

void NativeWidgetAndroid::Activate() {
}

void NativeWidgetAndroid::Deactivate() {
}

bool NativeWidgetAndroid::IsActive() const {
  return true;
}

void NativeWidgetAndroid::SetZOrderLevel(ui::ZOrderLevel order) {
}

ui::ZOrderLevel NativeWidgetAndroid::GetZOrderLevel() const {
  return ui::ZOrderLevel::kNormal;
}

void NativeWidgetAndroid::SetVisibleOnAllWorkspaces(bool always_visible) {
  // Not implemented on chromeos or for child widgets.
}

bool NativeWidgetAndroid::IsVisibleOnAllWorkspaces() const {
  return false;
}

void NativeWidgetAndroid::Maximize() {
}

void NativeWidgetAndroid::Minimize() {
}

bool NativeWidgetAndroid::IsMaximized() const {
  return true;
}

bool NativeWidgetAndroid::IsMinimized() const {
  return false;
}

void NativeWidgetAndroid::Restore() {
}

void NativeWidgetAndroid::SetFullscreen(bool fullscreen) {
}

bool NativeWidgetAndroid::IsFullscreen() const {
  return false;
}

void NativeWidgetAndroid::SetCanAppearInExistingFullscreenSpaces(
    bool can_appear_in_existing_fullscreen_spaces) {}

void NativeWidgetAndroid::SetOpacity(float opacity) {
}

void NativeWidgetAndroid::SetAspectRatio(const gfx::SizeF& aspect_ratio) {
}

void NativeWidgetAndroid::FlashFrame(bool flash) {
}

void NativeWidgetAndroid::RunShellDrag(View* view,
                                    std::unique_ptr<ui::OSExchangeData> data,
                                    const gfx::Point& location,
                                    int operation,
                                    ui::DragDropTypes::DragEventSource source) {
}

void NativeWidgetAndroid::SchedulePaintInRect(const gfx::Rect& rect) {
}

void NativeWidgetAndroid::ScheduleLayout() {
}

void NativeWidgetAndroid::SetCursor(gfx::NativeCursor cursor) {
}

bool NativeWidgetAndroid::IsMouseEventsEnabled() const {
  return true;
}

bool NativeWidgetAndroid::IsMouseButtonDown() const {
  return false;
}

void NativeWidgetAndroid::ClearNativeFocus() {
}

gfx::Rect NativeWidgetAndroid::GetWorkAreaBoundsInScreen() const {
  return gfx::Rect();
}

Widget::MoveLoopResult NativeWidgetAndroid::RunMoveLoop(
    const gfx::Vector2d& drag_offset,
    Widget::MoveLoopSource source,
    Widget::MoveLoopEscapeBehavior escape_behavior) {
  return Widget::MOVE_LOOP_CANCELED;
}

void NativeWidgetAndroid::EndMoveLoop() {
}

void NativeWidgetAndroid::SetVisibilityChangedAnimationsEnabled(bool value) {
}

void NativeWidgetAndroid::SetVisibilityAnimationDuration(
    const base::TimeDelta& duration) {
}

void NativeWidgetAndroid::SetVisibilityAnimationTransition(
    Widget::VisibilityTransition transition) {
}

bool NativeWidgetAndroid::IsTranslucentWindowOpacitySupported() const {
  return true;
}

ui::GestureRecognizer* NativeWidgetAndroid::GetGestureRecognizer() {
  return nullptr;
}

void NativeWidgetAndroid::OnSizeConstraintsChanged() {
}

std::string NativeWidgetAndroid::GetName() const {
  return std::string();
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetAndroid, protected:

NativeWidgetAndroid::~NativeWidgetAndroid() {
  destroying_ = true;
  if (ownership_ == Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET)
    delete delegate_;
  else
    CloseNow();
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetAndroid, private:

void NativeWidgetAndroid::SetInitialFocus(ui::WindowShowState show_state) {
}

////////////////////////////////////////////////////////////////////////////////
// Widget, public:

namespace {
#if defined(OS_WIN) || defined(USE_X11)
void CloseWindow(gfx::NativeWindow window) {
  if (window) {
    Widget* widget = Widget::GetWidgetForNativeView(window);
    if (widget && widget->is_secondary_widget())
      // To avoid the delay in shutdown caused by using Close which may wait
      // for animations, use CloseNow. Because this is only used on secondary
      // widgets it seems relatively safe to skip the extra processing of
      // Close.
      widget->CloseNow();
  }
}
#endif

#if defined(OS_WIN)
BOOL CALLBACK WindowCallbackProc(HWND hwnd, LPARAM lParam) {
  gfx::NativeWindow root_window =
      DesktopWindowTreeHostWin::GetContentWindowForHWND(hwnd);
  CloseWindow(root_window);
  return TRUE;
}
#endif
}  // namespace

// static
void Widget::CloseAllSecondaryWidgets() {
#if defined(OS_WIN)
  EnumThreadWindows(GetCurrentThreadId(), WindowCallbackProc, 0);
#endif

#if defined(USE_X11)
  DesktopWindowTreeHostX11::CleanUpWindowList(CloseWindow);
#endif
}

namespace internal {

////////////////////////////////////////////////////////////////////////////////
// internal::NativeWidgetPrivate, public:

// static
NativeWidgetPrivate* NativeWidgetPrivate::CreateNativeWidget(
    internal::NativeWidgetDelegate* delegate) {
  return new NativeWidgetAndroid(delegate);
}

// static
NativeWidgetPrivate* NativeWidgetPrivate::GetNativeWidgetForNativeView(
    gfx::NativeView native_view) {
  return nullptr;
}

// static
NativeWidgetPrivate* NativeWidgetPrivate::GetNativeWidgetForNativeWindow(
    gfx::NativeWindow native_window) {
  return nullptr;
}

// static
NativeWidgetPrivate* NativeWidgetPrivate::GetTopLevelNativeWidget(
    gfx::NativeView native_view) {
  return nullptr;
}

// static
void NativeWidgetPrivate::GetAllChildWidgets(gfx::NativeView native_view,
                                             Widget::Widgets* children) {
}

// static
void NativeWidgetPrivate::GetAllOwnedWidgets(gfx::NativeView native_view,
                                             Widget::Widgets* owned) {
}

// static
void NativeWidgetPrivate::ReparentNativeView(gfx::NativeView native_view,
                                             gfx::NativeView new_parent) {
}

// static
gfx::NativeView NativeWidgetPrivate::GetGlobalCapture(
    gfx::NativeView native_view) {
  return nullptr;
}

}  // namespace internal
}  // namespace views
