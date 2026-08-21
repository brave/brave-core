// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_view_manager.h"

#include "base/rand_util.h"
#include "brave/components/brave_search/browser/prefs.h"
#include "brave/components/brave_search/common/features.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "brave/android/java/org/chromium/chrome/browser/brave_search/jni_headers/BackupResultsWindowFactory_jni.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#elif defined(USE_AURA)
#include "ui/aura/client/window_types.h"
#include "ui/aura/window.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/layer_type.h"
#include "ui/gfx/geometry/point.h"
#endif

namespace brave_search {

namespace {

gfx::Size GetViewSize(PrefService* local_state) {
  if (features::kBackupResultsZeroSize.Get()) {
    return gfx::Size();
  }
  const int stored_width =
      local_state->GetInteger(prefs::kBackupResultsLastViewWidth);
  const int stored_height =
      local_state->GetInteger(prefs::kBackupResultsLastViewHeight);
  return gfx::Size(
      stored_width > 0 ? stored_width : base::RandIntInclusive(800, 1920),
      stored_height > 0 ? stored_height : base::RandIntInclusive(600, 1080));
}

#if BUILDFLAG(IS_MAC) || defined(USE_AURA)
gfx::Rect GetWindowBounds(PrefService* local_state,
                          const gfx::Size& view_size) {
  // A zero-sized view must not be paired with a non-zero window.
  if (view_size.IsEmpty()) {
    return gfx::Rect();
  }
  gfx::Size window_size(
      local_state->GetInteger(prefs::kBackupResultsLastWindowWidth),
      local_state->GetInteger(prefs::kBackupResultsLastWindowHeight));
  // The window must be able to contain the view.
  window_size.SetToMax(view_size);
  return gfx::Rect(
      gfx::Point(local_state->GetInteger(prefs::kBackupResultsLastWindowX),
                 local_state->GetInteger(prefs::kBackupResultsLastWindowY)),
      window_size);
}
#endif

}  // namespace

BackupResultsViewManager::BackupResultsViewManager(
    PrefService* local_state,
    content::WebContents* web_contents) {
  const gfx::Size view_size = GetViewSize(local_state);
#if BUILDFLAG(IS_MAC) || defined(USE_AURA)
  const gfx::Rect window_bounds = GetWindowBounds(local_state, view_size);
#endif

#if BUILDFLAG(IS_ANDROID)
  auto* native_view = web_contents->GetNativeView();
  window_ = ui::WindowAndroid::FromJavaWindowAndroid(
      Java_BackupResultsWindowFactory_create(
          base::android::AttachCurrentThread()));
  if (window_) {
    window_->AddChild(native_view);
  }
  const float dip_scale = native_view->GetDipScale();
  native_view->OnSizeChanged(static_cast<int>(view_size.width() * dip_scale),
                             static_cast<int>(view_size.height() * dip_scale));
#elif BUILDFLAG(IS_MAC)
  window_ =
      BackupResultsWindowMac::Create(web_contents, window_bounds, view_size);
#elif defined(USE_AURA)
  window_ =
      std::make_unique<aura::Window>(this, aura::client::WINDOW_TYPE_CONTROL);
  window_->Init(ui::LAYER_NOT_DRAWN);
  window_->SetBounds(window_bounds);
  window_->Show();

  auto* native_view = web_contents->GetNativeView();
  window_->AddChild(native_view);

  // Align the view with the bottom left of the window, so that the difference
  // in height is attributed to browser chrome.
  native_view->SetBounds(gfx::Rect(
      gfx::Point(0, window_bounds.height() - view_size.height()), view_size));
  native_view->Show();
#else
  web_contents->Resize(gfx::Rect(view_size));
#endif
}

BackupResultsViewManager::~BackupResultsViewManager() {
#if BUILDFLAG(IS_ANDROID)
  if (!window_) {
    return;
  }
  auto java_window = window_->GetJavaObject();
  window_ = nullptr;
  Java_BackupResultsWindowFactory_destroy(base::android::AttachCurrentThread(),
                                          java_window);
#endif
}

#if defined(USE_AURA)

gfx::Size BackupResultsViewManager::GetMinimumSize() const {
  return gfx::Size();
}

std::optional<gfx::Size> BackupResultsViewManager::GetMaximumSize() const {
  return std::nullopt;
}

gfx::NativeCursor BackupResultsViewManager::GetCursor(const gfx::Point& point) {
  return gfx::NativeCursor{};
}

int BackupResultsViewManager::GetNonClientComponent(
    const gfx::Point& point) const {
  return HTCLIENT;
}

bool BackupResultsViewManager::ShouldDescendIntoChildForEventHandling(
    aura::Window* child,
    const gfx::Point& location) {
  return true;
}

bool BackupResultsViewManager::CanFocus() {
  return true;
}

bool BackupResultsViewManager::HasHitTestMask() const {
  return false;
}

#endif

}  // namespace brave_search
