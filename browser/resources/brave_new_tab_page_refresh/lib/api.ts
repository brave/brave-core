/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// An API is an interface that encapsulates a set of actions and a current
// state. The state may be accessed at any time, and listeners can be notified
// when the state has changed.
export type API<State, Actions> = Actions & {
  getState: () => State,
  addListener: (listener: (state: State) => void) => void
}
