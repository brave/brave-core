/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/webcompatreporter_types'

const defaultState: WebcompatReporter.State = {
  dialogArgs: {
    url: '',
    adBlockSetting: '',
    fpBlockSetting: '',
    shieldsEnabled: ''
  },
  submitted: false
}

const webcompatReporterReducer: Reducer<WebcompatReporter.State | undefined> = (state: WebcompatReporter.State = defaultState, action) => {
  switch (action.type) {
    case types.WEBCOMPATREPORTER_ON_SUBMIT_REPORT:
      chrome.send('webcompat_reporter.submitReport', [
        {
          ...state.dialogArgs,
          additionalDetails: action.payload.details || null,
          contactInfo: action.payload.contact || null
        }
      ])
      state = {
        ...state,
        submitted: true
      }
      setTimeout(() => chrome.send('dialogClose'), 5000)
      break
    case types.WEBCOMPATREPORTER_SET_DIALOG_ARGS:
      state = {
        ...state,
        dialogArgs: action.payload.dialogArgs
      }
      break
    case types.WEBCOMPATREPORTER_ON_CLOSE:
      chrome.send('dialogClose')
      break
    default:
      break
  }

  return state
}

export default webcompatReporterReducer
