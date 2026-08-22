// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_VIEW_MANAGER_H_
#define BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_VIEW_MANAGER_H_

#include <memory>
#include <optional>

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/memory/raw_ptr.h"
#elif BUILDFLAG(IS_MAC)
#include "brave/browser/brave_search/backup_results_window_mac.h"
#elif defined(USE_AURA)
#include "ui/aura/window_delegate.h"
#endif

class PrefService;

namespace content {
class WebContents;
}  // namespace content

#if BUILDFLAG(IS_ANDROID)
namespace ui {
class WindowAndroid;
}  // namespace ui
#endif

namespace brave_search {

// Sets up the view of an offscreen backup results WebContents to resemble a
// real browser tab. Must outlive that WebContents.
class BackupResultsViewManager
#if defined(USE_AURA)
    : public aura::WindowDelegate
#endif
{
 public:
  BackupResultsViewManager(PrefService* local_state,
                           content::WebContents* web_contents);

#if defined(USE_AURA)
  ~BackupResultsViewManager() override;
#else
  ~BackupResultsViewManager();
#endif

  BackupResultsViewManager(const BackupResultsViewManager&) = delete;
  BackupResultsViewManager& operator=(const BackupResultsViewManager&) = delete;

 private:
#if defined(USE_AURA)
  // aura::WindowDelegate:
  gfx::Size GetMinimumSize() const override;
  std::optional<gfx::Size> GetMaximumSize() const override;
  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {}
  gfx::NativeCursor GetCursor(const gfx::Point& point) override;
  int GetNonClientComponent(const gfx::Point& point) const override;
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override;
  bool CanFocus() override;
  void OnCaptureLost() override {}
  void OnPaint(const ui::PaintContext& context) override {}
  void OnDeviceScaleFactorChanged(float old_device_scale_factor,
                                  float new_device_scale_factor) override {}
  void OnWindowDestroying(aura::Window* window) override {}
  void OnWindowDestroyed(aura::Window* window) override {}
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override;
  void GetHitTestMask(SkPath* mask) const override {}
#endif

#if BUILDFLAG(IS_ANDROID)
  raw_ptr<ui::WindowAndroid> window_ = nullptr;
#elif BUILDFLAG(IS_MAC)
  std::unique_ptr<BackupResultsWindowMac> window_;
#elif defined(USE_AURA)
  std::unique_ptr<aura::Window> window_;
#endif
};

}  // namespace brave_search

#endif  // BRAVE_BROWSER_BRAVE_SEARCH_BACKUP_RESULTS_VIEW_MANAGER_H_
