/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/gemini_types'

const geminiReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_VALID_GEMINI_AUTH_CODE:
      state = { ...state }
      state.geminiState.userAuthed = true
      state.geminiState.authInProgress = false
      break

    case types.CONNECT_TO_GEMINI:
      state = { ...state }
      state.geminiState.authInProgress = true
      break

    case types.ON_GEMINI_CLIENT_URL:
      const { clientUrl } = payload

      state = { ...state }
      state.geminiState.geminiClientUrl = clientUrl
      break

    default:
      break
  }

  return state
}

export default geminiReducer
