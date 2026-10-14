// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/test/fake_mouse_rocker_wheel_default_commands.h"

#include "brave/components/gestures/core/browser/mouse_rocker_default_commands_info.h"
#include "brave/components/gestures/core/browser/mouse_wheel_default_commands_info.h"

namespace gestures::test {

namespace {

constexpr int kFakeRockLeftCommand = 1000001;
constexpr int kFakeRockRightCommand = 1000002;
constexpr int kFakeWheelUpCommand = 1000003;
constexpr int kFakeWheelDownCommand = 1000004;

}  // namespace

MouseRockerDefaultCommandsInfo FakeMouseRockerDefaultCommands() {
  return {
      .left = kFakeRockLeftCommand,
      .right = kFakeRockRightCommand,
  };
}

MouseWheelDefaultCommandsInfo FakeMouseWheelDefaultCommands() {
  return {
      .up = kFakeWheelUpCommand,
      .down = kFakeWheelDownCommand,
  };
}

}  // namespace gestures::test
