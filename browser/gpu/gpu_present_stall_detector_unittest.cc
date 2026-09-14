/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/gpu/gpu_present_stall_detector.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {
namespace {

constexpr base::TimeDelta kStallTimeout = base::Seconds(15);
constexpr base::TimeDelta kCooldown = base::Seconds(60);

TEST(GpuPresentStallDetectorTest, NoRestartWhenPresentsKeepUp) {
  const base::TimeTicks t0 = base::TimeTicks() + base::Seconds(100);
  GpuPresentStallInput input;
  input.compositor_visible = true;
  input.now = t0;
  EXPECT_FALSE(ShouldRestartGpuProcessForPresentStall(input, kStallTimeout,
                                                      kCooldown));
}

TEST(GpuPresentStallDetectorTest, NoRestartBeforeTimeout) {
  const base::TimeTicks t0 = base::TimeTicks() + base::Seconds(100);
  GpuPresentStallInput input;
  input.compositor_visible = true;
  input.now = t0 + base::Seconds(14);
  input.unacked_since = t0;
  EXPECT_FALSE(ShouldRestartGpuProcessForPresentStall(input, kStallTimeout,
                                                      kCooldown));
}

TEST(GpuPresentStallDetectorTest, RestartAfterPresentStall) {
  const base::TimeTicks t0 = base::TimeTicks() + base::Seconds(100);
  GpuPresentStallInput input;
  input.compositor_visible = true;
  input.now = t0 + base::Seconds(15);
  input.unacked_since = t0;
  EXPECT_TRUE(ShouldRestartGpuProcessForPresentStall(input, kStallTimeout,
                                                     kCooldown));
}

TEST(GpuPresentStallDetectorTest, NoRestartWhenCompositorHidden) {
  const base::TimeTicks t0 = base::TimeTicks() + base::Seconds(100);
  GpuPresentStallInput input;
  input.compositor_visible = false;
  input.now = t0 + base::Seconds(30);
  input.unacked_since = t0;
  EXPECT_FALSE(ShouldRestartGpuProcessForPresentStall(input, kStallTimeout,
                                                      kCooldown));
}

TEST(GpuPresentStallDetectorTest, CooldownPreventsImmediateRerestart) {
  const base::TimeTicks t0 = base::TimeTicks() + base::Seconds(100);
  GpuPresentStallInput input;
  input.compositor_visible = true;
  input.now = t0 + base::Seconds(20);
  input.unacked_since = t0;
  input.last_restart = t0 + base::Seconds(15);
  EXPECT_FALSE(ShouldRestartGpuProcessForPresentStall(input, kStallTimeout,
                                                      kCooldown));

  input.now = input.last_restart + kCooldown;
  EXPECT_TRUE(ShouldRestartGpuProcessForPresentStall(input, kStallTimeout,
                                                     kCooldown));
}

}  // namespace
}  // namespace brave
