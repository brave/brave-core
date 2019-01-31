/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
// Temporary (ryanml)
import { types } from '../constants/rewards_types'

const grantReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.GET_GRANTS:
      chrome.send('brave_rewards.getGrants', [])
      break
    case types.ON_GRANT:
      state = { ...state }
      if (action.payload.properties.status === 1) {
        break
      }

      if (!state.grants) {
        state.grants = []
      }

      state.grants.push({
        promotionId: action.payload.properties.promotionId,
        expiryTime: 0,
        probi: '',
        type: action.payload.properties.type
      })

      state = {
        ...state,
        grants: state.grants
      }

      break
    case types.GET_GRANT_CAPTCHA:
      chrome.send('brave_rewards.getGrantCaptcha', [])
      break
    case types.ON_GRANT_CAPTCHA:
      {
        // Temporary (ryanml)
        /*
        if (state.grant) {
          let grant = state.grant
          const props = action.payload.captcha
          grant.captcha = `data:image/jpeg;base64,${props.image}`
          grant.hint = props.hint
          state = {
            ...state,
            grant
          }
        }

        break
        */
      }
    case types.SOLVE_GRANT_CAPTCHA:
      if (action.payload.x && action.payload.y) {
        chrome.send('brave_rewards.solveGrantCaptcha', [JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        })])
      }
      break
    case types.ON_GRANT_RESET:
      {
        // Temporary (ryanml)
        /*
        if (state.grant) {
          const grant: Rewards.Grant = {
            promotionId: state.grant.promotionId,
            probi: '',
            expiryTime: 0
          }

          state = {
            ...state,
            grant
          }
        }

        break
        */
      }
    case types.ON_GRANT_DELETE:
      {
        // Temporary (ryanml)
        /*
        if (state.grant) {
          delete state.grant

          state = {
            ...state
          }
        }

        break
        */
      }
    case types.ON_GRANT_FINISH:
      {
        // Temporary (ryanml)
        /*
        state = { ...state }
        const properties: Rewards.Grant = action.payload.properties
        // TODO NZ check why enum can't be used inside Rewards namespace
        if (properties.status === 0) {
          if (state.grant) {
            let grant = state.grant
            let ui = state.ui
            grant.expiryTime = properties.expiryTime * 1000
            grant.probi = properties.probi
            grant.status = null
            ui.emptyWallet = false

            state = {
              ...state,
              grant,
              ui
            }
            chrome.send('brave_rewards.getWalletProperties', [])
          }
        } else if (properties.status === 6) {
          state = { ...state }
          if (state.grant) {
            let grant = state.grant
            grant.status = 'wrongPosition'

            state = {
              ...state,
              grant
            }
          }
          chrome.send('brave_rewards.getGrantCaptcha', [])
        } else if (properties.status === 13) {
          state = { ...state }
          if (state.grant) {
            let grant = state.grant
            grant.status = 'grantGone'

            state = {
              ...state,
              grant
            }
          }
        } else if (properties.status === 18) {
          state = { ...state }
          if (state.grant) {
            let grant = state.grant
            grant.status = 'grantAlreadyClaimed'

            state = {
              ...state,
              grant
            }
          }
        } else {
          state = { ...state }
          if (state.grant) {
            let grant = state.grant
            grant.status = 'generalError'

            state = {
              ...state,
              grant
            }
          }
        }
        break
        */
      }
  }

  return state
}

export default grantReducer
