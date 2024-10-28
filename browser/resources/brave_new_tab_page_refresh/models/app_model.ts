/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BackgroundState,
  BackgroundActions,
  defaultBackgroundState } from './backgrounds'

export type AppState =
  BackgroundState

export function defaultState(): AppState {
  return {
    ...defaultBackgroundState()
  }
}

export type AppActions =
  BackgroundActions

export interface AppModel extends AppActions {
  getState: () => AppState
  addListener: (listener: (state: AppState) => void) => () => void
}
