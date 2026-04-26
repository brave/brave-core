/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createSlice, PayloadAction } from '@reduxjs/toolkit'

import { closeDialog, submitReport } from '../browser_proxy'
import type { AppThunk } from '../hooks'

const initialState: WebcompatReporter.State = {
  dialogArgs: {
    url: '',
    isErrorPage: false,
    adBlockSetting: '',
    fpBlockSetting: '',
    shieldsEnabled: '',
    contactInfo: '',
    contactInfoSaveFlag: false,
    components: []
  },
  submitted: false
}

const webcompatReporterSlice = createSlice({
  name: 'webcompatReporter',
  initialState,
  reducers: {
    setDialogArgs(state, action: PayloadAction<WebcompatReporter.DialogArgs>) {
      state.dialogArgs = action.payload
    },
    setSubmitted(state, action: PayloadAction<boolean>) {
      state.submitted = action.payload
    }
  }
})

// Internal actions from the slice
const { setSubmitted } = webcompatReporterSlice.actions

// Re-export setDialogArgs directly as it has no side effects
export const { setDialogArgs } = webcompatReporterSlice.actions

// Thunk actions that handle side effects
export const onSubmitReport =
  (
    category: string,
    details: string,
    contact: string,
    attachScreenshot: boolean
  ): AppThunk =>
  (dispatch, getState) => {
    const state = getState()
    const dialogArgs = state.reporterState?.dialogArgs

    if (dialogArgs) {
      submitReport({
        ...dialogArgs,
        category: category || null,
        additionalDetails: details || null,
        contactInfo: contact || null,
        attachScreenshot: attachScreenshot || false
      })
    }

    dispatch(setSubmitted(true))
    setTimeout(closeDialog, 5000)
  }

export const onClose = (): AppThunk => () => {
  closeDialog()
}

export default webcompatReporterSlice.reducer
