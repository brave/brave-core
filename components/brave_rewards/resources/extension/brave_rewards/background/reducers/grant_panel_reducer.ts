/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'

export const grantPanelReducer = (state: RewardsExtension.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  switch (action.type) {
    case types.ON_GRANT:
      state = { ...state }
      if (action.payload.properties.status === 1) {
        state.grant = undefined
        break
      }

      state.grant = {
        promotionId: action.payload.properties.promotionId,
        expiryTime: 0,
        probi: ''
      }
      break
    case types.GET_GRANT_CAPTCHA:
      chrome.braveRewards.getGrantCaptcha()
      break
    case types.ON_GRANT_CAPTCHA:
      {
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
      }
    case types.SOLVE_GRANT_CAPTCHA:
      if (action.payload.x && action.payload.y) {
        chrome.braveRewards.solveGrantCaptcha(JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        }))
      }
      break
    case types.ON_GRANT_RESET:
      {
        if (state.grant) {
          const grant: RewardsExtension.GrantInfo = {
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
      }
    case types.ON_GRANT_DELETE:
      {
        if (state.grant) {
          delete state.grant

          state = {
            ...state
          }
        }

        break
      }
    case types.ON_GRANT_FINISH:
      {
        state = { ...state }
        const properties: RewardsExtension.GrantInfo = action.payload.properties
        if (properties.status === 0) {
          if (state.grant) {
            let grant = state.grant
            grant.expiryTime = properties.expiryTime * 1000
            grant.probi = properties.probi
            grant.status = null

            state = {
              ...state,
              grant
            }
           chrome.braveRewards.getWalletProperties()
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
          chrome.braveRewards.getGrantCaptcha()
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
      }
    }

  return state
}
