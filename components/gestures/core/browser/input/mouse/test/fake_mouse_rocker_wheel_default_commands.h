// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_TEST_FAKE_MOUSE_ROCKER_WHEEL_DEFAULT_COMMANDS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_TEST_FAKE_MOUSE_ROCKER_WHEEL_DEFAULT_COMMANDS_H_

namespace gestures {

struct MouseRockerDefaultCommandsInfo;
struct MouseWheelDefaultCommandsInfo;

}  // namespace gestures

namespace gestures::test {

// Fake rocker left/right command ids, distinct from any real `IDC_*` value,
// for tests that only check `RegisterProfilePrefs` passes defaults through.
MouseRockerDefaultCommandsInfo FakeMouseRockerDefaultCommands();

// Fake wheel up/down command ids; see `FakeMouseRockerDefaultCommands()`.
MouseWheelDefaultCommandsInfo FakeMouseWheelDefaultCommands();

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_INPUT_MOUSE_TEST_FAKE_MOUSE_ROCKER_WHEEL_DEFAULT_COMMANDS_H_
