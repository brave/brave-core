/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Mirrors `SmartGestureSelectedTextSearchCommandParamType` in
// `smart_gesture_selected_text_search_command_param_types.h`; persisted in
// prefs, so never renumber existing entries, only append new ones.
export enum SmartGestureSelectedTextSearchCommandParamType {
  BRAVE_SEARCH = 0,
  BRAVE_ASK = 1,
}

export const SEARCH_TARGET_ORDER: SmartGestureSelectedTextSearchCommandParamType[] = [
  SmartGestureSelectedTextSearchCommandParamType.BRAVE_SEARCH,
  SmartGestureSelectedTextSearchCommandParamType.BRAVE_ASK,
]

export const SEARCH_TARGET_I18N_KEYS:
  Record<SmartGestureSelectedTextSearchCommandParamType, string> = {
    [SmartGestureSelectedTextSearchCommandParamType.BRAVE_SEARCH]:
      'braveGesturesSearchTargetBraveSearch',
    [SmartGestureSelectedTextSearchCommandParamType.BRAVE_ASK]:
      'braveGesturesSearchTargetBraveAsk',
  }
