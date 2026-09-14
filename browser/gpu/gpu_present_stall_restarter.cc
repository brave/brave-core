/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/gpu/gpu_present_stall_restarter.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/no_destructor.h"
#include "brave/browser/brave_browser_features.h"
#include "brave/browser/gpu/gpu_present_stall_detector.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "ui/compositor/compositor.h"
#include "ui/views/widget/widget.h"

namespace brave {
namespace {

base::TimeDelta StallTimeout() {
  return base::Seconds(
      features::kBraveRestartGpuOnPresentStallTimeoutSeconds.Get());
}

base::TimeDelta Cooldown() {
  return base::Seconds(
      features::kBraveRestartGpuOnPresentStallCooldownSeconds.Get());
}

base::TimeDelta CheckInterval() {
  return base::Seconds(
      features::kBraveRestartGpuOnPresentStallCheckIntervalSeconds.Get());
}

}  // namespace

// static
GpuPresentStallRestarter* GpuPresentStallRestarter::GetInstance() {
  static base::NoDestructor<GpuPresentStallRestarter> instance;
  return instance.get();
}

GpuPresentStallRestarter::GpuPresentStallRestarter() = default;

GpuPresentStallRestarter::~GpuPresentStallRestarter() {
  Stop();
}

void GpuPresentStallRestarter::Start() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (started_ ||
      !base::FeatureList::IsEnabled(features::kBraveRestartGpuOnPresentStall)) {
    return;
  }
  started_ = true;
  ObserveDesktopCompositors();
  check_timer_.Start(FROM_HERE, CheckInterval(), this,
                     &GpuPresentStallRestarter::CheckForStall);
}

void GpuPresentStallRestarter::Stop() {
  check_timer_.Stop();
  for (auto& [compositor, state] : compositor_state_) {
    if (state.observing && compositor) {
      compositor->RemoveObserver(this);
    }
  }
  compositor_state_.clear();
  started_ = false;
}

void GpuPresentStallRestarter::OnCompositingDidCommit(
    ui::Compositor* compositor) {
  auto& state = compositor_state_[compositor];
  if (state.unacked_since.is_null()) {
    state.unacked_since = base::TimeTicks::Now();
  }
}

void GpuPresentStallRestarter::OnDidPresentCompositorFrame(
    ui::Compositor* compositor,
    uint32_t /*frame_token*/,
    const gfx::PresentationFeedback& /*feedback*/) {
  compositor_state_[compositor].unacked_since = base::TimeTicks();
}

void GpuPresentStallRestarter::OnCompositingShuttingDown(
    ui::Compositor* compositor) {
  StopObserving(compositor);
}

void GpuPresentStallRestarter::OnCompositorVisibilityChanged(
    ui::Compositor* compositor,
    bool visible) {
  auto& state = compositor_state_[compositor];
  state.visible = visible;
  if (!visible) {
    state.unacked_since = base::TimeTicks();
  }
}

void GpuPresentStallRestarter::ObserveDesktopCompositors() {
  for (Browser* browser : *BrowserList::GetInstance()) {
    BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser);
    if (!browser_view) {
      continue;
    }
    views::Widget* widget = browser_view->GetWidget();
    if (!widget) {
      continue;
    }
    ui::Compositor* compositor = widget->GetCompositor();
    if (!compositor) {
      continue;
    }
    const bool visible = widget->IsVisible() && !widget->IsMinimized();
    EnsureObserving(compositor, visible);
  }
}

void GpuPresentStallRestarter::EnsureObserving(ui::Compositor* compositor,
                                               bool visible) {
  auto& state = compositor_state_[compositor];
  state.visible = visible;
  if (state.observing) {
    return;
  }
  compositor->AddObserver(this);
  state.observing = true;
}

void GpuPresentStallRestarter::StopObserving(ui::Compositor* compositor) {
  auto it = compositor_state_.find(compositor);
  if (it == compositor_state_.end()) {
    return;
  }
  if (it->second.observing && compositor) {
    compositor->RemoveObserver(this);
  }
  compositor_state_.erase(it);
}

void GpuPresentStallRestarter::CheckForStall() {
  ObserveDesktopCompositors();

  const base::TimeTicks now = base::TimeTicks::Now();
  const base::TimeDelta stall_timeout = StallTimeout();
  const base::TimeDelta cooldown = Cooldown();
  for (const auto& [compositor, state] : compositor_state_) {
    GpuPresentStallInput input;
    input.compositor_visible = state.visible;
    input.now = now;
    input.unacked_since = state.unacked_since;
    input.last_restart = last_restart_;
    if (ShouldRestartGpuProcessForPresentStall(input, stall_timeout,
                                               cooldown)) {
      RestartGpuProcess();
      return;
    }
  }
}

void GpuPresentStallRestarter::RestartGpuProcess() {
  last_restart_ = base::TimeTicks::Now();
  for (auto& [compositor, state] : compositor_state_) {
    state.unacked_since = base::TimeTicks();
  }

  LOG(WARNING) << "Restarting GPU process after compositor present stall";
  UMA_HISTOGRAM_BOOLEAN("Brave.GPU.PresentStallRestart", true);

  auto* host = content::GpuProcessHost::Get(
      content::GPU_PROCESS_KIND_SANDBOXED, /*force_create=*/false);
  if (host) {
    host->ForceShutdown();
  }
}

}  // namespace brave
