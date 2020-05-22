// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_NATIVE_WIDGET_ANDROID_H_
#define UI_VIEWS_WIDGET_NATIVE_WIDGET_ANDROID_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event_constants.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/wm/public/activation_change_observer.h"
#include "ui/wm/public/activation_delegate.h"

#if defined(OS_MACOSX)
#error This file must not be included on macOS; Chromium Mac doesn't use Aura.
#endif

namespace aura {
class Window;
}

namespace views {

class DropHelper;
class FocusManagerEventHandler;
class TooltipManagerAura;
class WindowReorderer;

class VIEWS_EXPORT NativeWidgetAndroid : public internal::NativeWidgetPrivate {
 public:
  explicit NativeWidgetAndroid(internal::NativeWidgetDelegate* delegate);

  // Called internally by NativeWidgetAndroid and DesktopNativeWidgetAndroid to
  // associate |native_widget| with |window|.
  static void RegisterNativeWidgetForWindow(
      internal::NativeWidgetPrivate* native_widget,
      gfx::NativeWindow window);

  // Assign an icon to aura window.
  static void AssignIconToAuraWindow(gfx::NativeWindow window,
                                     const gfx::ImageSkia& window_icon,
                                     const gfx::ImageSkia& app_icon);

  // If necessary, sets the ShadowElevation of |window| from |params|.
  static void SetShadowElevationFromInitParams(
      gfx::NativeWindow window,
      const Widget::InitParams& params);

  // Sets the window property aura::client::kResizeBehaviorKey based on the
  // values from the delegate.
  static void SetResizeBehaviorFromDelegate(WidgetDelegate* delegate,
                                            gfx::NativeWindow window);

  // Overridden from internal::NativeWidgetPrivate:
  void InitNativeWidget(Widget::InitParams params) override;
  void OnWidgetInitDone() override;
  NonClientFrameView* CreateNonClientFrameView() override;
  bool ShouldUseNativeFrame() const override;
  bool ShouldWindowContentsBeTransparent() const override;
  void FrameTypeChanged() override;
  Widget* GetWidget();
  const Widget* GetWidget() const;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  Widget* GetTopLevelWidget() override;
  const ui::Compositor* GetCompositor() const override;
  const ui::Layer* GetLayer() const override;
  void ReorderNativeViews() override;
  void ViewRemoved(View* view) override;
  void SetNativeWindowProperty(const char* name, void* value) override;
  void* GetNativeWindowProperty(const char* name) const override;
  TooltipManager* GetTooltipManager() const override;
  void SetCapture() override;
  void ReleaseCapture() override;
  bool HasCapture() const override;
  ui::InputMethod* GetInputMethod() override;
  void CenterWindow(const gfx::Size& size) override;
  void GetWindowPlacement(gfx::Rect* bounds,
                          ui::WindowShowState* maximized) const override;
  bool SetWindowTitle(const base::string16& title) override;
  void SetWindowIcons(const gfx::ImageSkia& window_icon,
                      const gfx::ImageSkia& app_icon) override;
  void InitModalType(ui::ModalType modal_type) override;
  gfx::Rect GetWindowBoundsInScreen() const override;
  gfx::Rect GetClientAreaBoundsInScreen() const override;
  gfx::Rect GetRestoredBounds() const override;
  std::string GetWorkspace() const override;
  void SetBounds(const gfx::Rect& bounds) override;
  void SetBoundsConstrained(const gfx::Rect& bounds) override;
  void SetSize(const gfx::Size& size) override;
  void StackAbove(gfx::NativeView native_view) override;
  void StackAtTop() override;
  void SetShape(std::unique_ptr<Widget::ShapeRects> shape) override;
  void Close() override;
  void CloseNow() override;
  void Show(ui::WindowShowState show_state,
            const gfx::Rect& restore_bounds) override;
  void Hide() override;
  bool IsVisible() const override;
  void Activate() override;
  void Deactivate() override;
  bool IsActive() const override;
  void SetZOrderLevel(ui::ZOrderLevel order) override;
  ui::ZOrderLevel GetZOrderLevel() const override;
  void SetVisibleOnAllWorkspaces(bool always_visible) override;
  bool IsVisibleOnAllWorkspaces() const override;
  void Maximize() override;
  void Minimize() override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  void Restore() override;
  void SetFullscreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetCanAppearInExistingFullscreenSpaces(
      bool can_appear_in_existing_fullscreen_spaces) override;
  void SetOpacity(float opacity) override;
  void SetAspectRatio(const gfx::SizeF& aspect_ratio) override;
  void FlashFrame(bool flash_frame) override;
  void RunShellDrag(View* view,
                    std::unique_ptr<ui::OSExchangeData> data,
                    const gfx::Point& location,
                    int operation,
                    ui::DragDropTypes::DragEventSource source) override;
  void SchedulePaintInRect(const gfx::Rect& rect) override;
  void ScheduleLayout() override;
  void SetCursor(gfx::NativeCursor cursor) override;
  bool IsMouseEventsEnabled() const override;
  bool IsMouseButtonDown() const override;
  void ClearNativeFocus() override;
  gfx::Rect GetWorkAreaBoundsInScreen() const override;
  Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      Widget::MoveLoopSource source,
      Widget::MoveLoopEscapeBehavior escape_behavior) override;
  void EndMoveLoop() override;
  void SetVisibilityChangedAnimationsEnabled(bool value) override;
  void SetVisibilityAnimationDuration(const base::TimeDelta& duration) override;
  void SetVisibilityAnimationTransition(
      Widget::VisibilityTransition transition) override;
  bool IsTranslucentWindowOpacitySupported() const override;
  ui::GestureRecognizer* GetGestureRecognizer() override;
  void OnSizeConstraintsChanged() override;
  std::string GetName() const override;
  void OnNativeViewHierarchyWillChange() override {}
  void OnNativeViewHierarchyChanged() override {}
  const Widget* GetWidgetImpl() const override;

 protected:
  ~NativeWidgetAndroid() override;

  internal::NativeWidgetDelegate* delegate() { return delegate_; }

 private:
  void SetInitialFocus(ui::WindowShowState show_state);

  internal::NativeWidgetDelegate* delegate_;

  // WARNING: set to NULL when destroyed. As the Widget is not necessarily
  // destroyed along with |window_| all usage of |window_| should first verify
  // non-NULL.
  gfx::NativeWindow window_;

  // See class documentation for Widget in widget.h for a note about ownership.
  Widget::InitParams::Ownership ownership_;

  // Are we in the destructor?
  bool destroying_;

  std::unique_ptr<TooltipManagerAura> tooltip_manager_;

  // Reorders child windows of |window_| associated with a view based on the
  // order of the associated views in the widget's view hierarchy.
  std::unique_ptr<WindowReorderer> window_reorderer_;

  std::unique_ptr<DropHelper> drop_helper_;

  // Native widget's handler to receive events before the event target.
  std::unique_ptr<FocusManagerEventHandler> focus_manager_event_handler_;

  // The following factory is used for calls to close the NativeWidgetAndroid
  // instance.
  base::WeakPtrFactory<NativeWidgetAndroid> close_widget_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(NativeWidgetAndroid);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_NATIVE_WIDGET_AURA_H_
