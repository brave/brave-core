// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export const setHistoryState = <T>(state: T) => {
  const oldState = typeof history.state === "object" ? history.state : {}
  const newState = {...oldState, ...state}
  history.replaceState(newState, document.title)
}

export const getHistoryValue = <T>(name: string, defaultValue: T): T => {
  const state = history.state ?? {}
  return name in state ? state[name] : defaultValue
}
