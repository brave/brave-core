// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createListenerMiddleware, TypedStartListening } from '@reduxjs/toolkit'

import type { State, Dispatch } from './types'

export const listenerMiddleware = createListenerMiddleware()

// TODO: When @reduxjs/toolkit is upgraded to 2.0+, migrate to the simpler
// withTypes pattern:
//   export const startAppListening =
//     listenerMiddleware.startListening.withTypes<State, Dispatch>()
export type AppStartListening = TypedStartListening<State, Dispatch>

export const startAppListening =
  listenerMiddleware.startListening as AppStartListening
