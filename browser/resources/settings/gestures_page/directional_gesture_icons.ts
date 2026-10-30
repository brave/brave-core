/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Leo icon names, not raw glyphs. Keep in sync with the key characters
// `DirectionalGestureStrokeToKeyCode` produces.
const GESTURE_KEY_ICON_NAMES: Record<string, string> = {
  '↑': 'arrow-up', '↓': 'arrow-down', '←': 'arrow-left', '→': 'arrow-right',
  '↖': 'arrow-diagonal-up-left', '↗': 'arrow-diagonal-up-right',
  '↙': 'arrow-diagonal-down-left', '↘': 'arrow-diagonal-down-right',
}

export function gestureKeyToIconNames(key: string): string[] {
  return key.split('').map((c) => GESTURE_KEY_ICON_NAMES[c] ?? c)
}
