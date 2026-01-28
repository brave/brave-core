/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  createSlice,
  createAction,
  createListenerMiddleware,
  PayloadAction
} from '@reduxjs/toolkit'
import * as storage from '../storage'

const initialState: TorInternals.State = storage.load()

// Request actions (trigger chrome.send side effects)
export const getTorGeneralInfo = createAction('torInternals/getTorGeneralInfo')
export const getTorLog = createAction('torInternals/getTorLog')

const torInternalsSlice = createSlice({
  name: 'torInternals',
  initialState,
  reducers: {
    onGetTorGeneralInfo(
      state,
      action: PayloadAction<{ generalInfo: TorInternals.GeneralInfo }>
    ) {
      state.generalInfo = action.payload.generalInfo
    },
    onGetTorLog(state, action: PayloadAction<{ log: string }>) {
      state.log = action.payload.log
    },
    onGetTorInitPercentage(
      state,
      action: PayloadAction<{ percentage: string }>
    ) {
      state.generalInfo.torInitPercentage = action.payload.percentage
    },
    onGetTorCircuitEstablished(
      state,
      action: PayloadAction<{ success: boolean }>
    ) {
      state.generalInfo.isTorConnected = action.payload.success
    },
    onGetTorControlEvent(state, action: PayloadAction<{ event: string }>) {
      state.torControlEvents.push(action.payload.event)
    }
  }
})

// Listener middleware for side effects
export const listenerMiddleware = createListenerMiddleware()

listenerMiddleware.startListening({
  actionCreator: getTorGeneralInfo,
  effect: () => {
    chrome.send('tor_internals.getTorGeneralInfo')
  }
})

listenerMiddleware.startListening({
  actionCreator: getTorLog,
  effect: () => {
    chrome.send('tor_internals.getTorLog')
  }
})

export const {
  onGetTorGeneralInfo,
  onGetTorLog,
  onGetTorInitPercentage,
  onGetTorCircuitEstablished,
  onGetTorControlEvent
} = torInternalsSlice.actions

export default torInternalsSlice.reducer
