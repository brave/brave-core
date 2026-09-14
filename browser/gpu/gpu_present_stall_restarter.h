/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_GPU_GPU_PRESENT_STALL_RESTARTER_H_
#define BRAVE_BROWSER_GPU_GPU_PRESENT_STALL_RESTARTER_H_

#include <cstdint>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "ui/compositor/compositor_observer.h"

namespace ui {
class Compositor;
}

namespace gfx {
struct PresentationFeedback;
}

namespace brave {

// Watches desktop UI compositors for "commit without present" stalls.
//
// Chromium's GPU watchdog only notices the GPU thread failing to process
// command buffers. On macOS the hang is often the GPU process main thread
// blocked in CATransaction/WindowServer, so the watchdog never fires and
// closing tabs cannot recover the UI. Restarting the GPU process from the
// browser process is the recovery Chromium already uses after a GPU crash.
class GpuPresentStallRestarter : public ui::CompositorObserver {
 public:
  static GpuPresentStallRestarter* GetInstance();

  GpuPresentStallRestarter(const GpuPresentStallRestarter&) = delete;
  GpuPresentStallRestarter& operator=(const GpuPresentStallRestarter&) = delete;

  void Start();
  void Stop();

  // ui::CompositorObserver:
  void OnCompositingDidCommit(ui::Compositor* compositor) override;
  void OnDidPresentCompositorFrame(
      ui::Compositor* compositor,
      uint32_t frame_token,
      const gfx::PresentationFeedback& feedback) override;
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;
  void OnCompositorVisibilityChanged(ui::Compositor* compositor,
                                     bool visible) override;

 private:
  friend class base::NoDestructor<GpuPresentStallRestarter>;

  struct CompositorState {
    bool visible = false;
    bool observing = false;
    base::TimeTicks unacked_since;
  };

  GpuPresentStallRestarter();
  ~GpuPresentStallRestarter() override;

  void ObserveDesktopCompositors();
  void EnsureObserving(ui::Compositor* compositor, bool visible);
  void StopObserving(ui::Compositor* compositor);
  void CheckForStall();
  void RestartGpuProcess();

  base::RepeatingTimer check_timer_;
  base::flat_map<raw_ptr<ui::Compositor>, CompositorState> compositor_state_;
  base::TimeTicks last_restart_;
  bool started_ = false;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_GPU_GPU_PRESENT_STALL_RESTARTER_H_
