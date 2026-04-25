/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { configureStore } from '@reduxjs/toolkit'
import torInternalsReducer, {
  listenerMiddleware
} from './slices/tor_internals.slice'

const store = configureStore({
  reducer: {
    torInternalsData: torInternalsReducer
  },
  middleware: (getDefaultMiddleware) =>
    getDefaultMiddleware().prepend(listenerMiddleware.middleware)
})

export default store
