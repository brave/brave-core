/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Mirrors `SmartGestureOpenInCommandParamType` in
// `smart_gesture_open_in_command_param_types.h`; persisted in prefs, so never
// renumber existing entries, only append new ones.
export enum SmartGestureOpenInCommandParamType {
  NEW_BACKGROUND_TAB = 0,
  NEW_FOREGROUND_TAB = 1,
  NEW_WINDOW = 2,
}

export const OPEN_TARGET_ORDER: SmartGestureOpenInCommandParamType[] = [
  SmartGestureOpenInCommandParamType.NEW_BACKGROUND_TAB,
  SmartGestureOpenInCommandParamType.NEW_FOREGROUND_TAB,
  SmartGestureOpenInCommandParamType.NEW_WINDOW,
]

export const OPEN_TARGET_I18N_KEYS:
  Record<SmartGestureOpenInCommandParamType, string> = {
  [SmartGestureOpenInCommandParamType.NEW_BACKGROUND_TAB]:
    'braveGesturesOpenTargetNewBackgroundTab',
  [SmartGestureOpenInCommandParamType.NEW_FOREGROUND_TAB]:
    'braveGesturesOpenTargetNewForegroundTab',
  [SmartGestureOpenInCommandParamType.NEW_WINDOW]:
    'braveGesturesOpenTargetNewWindow',
}
