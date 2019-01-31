/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
// Temporary (ryanml)
import { types } from '../constants/rewards_types'

const getGrant = (id?: string, grants?: Rewards.Grant[]) => {
  if (!id || !grants) {
    return null
  }

  return grants.find((grant: Rewards.Grant) => {
    return (grant.promotionId === id)
  })
}

const updateGrant = (newGrant: Rewards.Grant, grants: Rewards.Grant[]) => {
  return grants.map((grant: Rewards.Grant) => {
    if (newGrant.promotionId === grant.promotionId) {
      return Object.assign(newGrant, grant)
    }
    return grant
  })
}

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

      const promotionId = action.payload.properties.promotionId

      if (!getGrant(promotionId, state.grants)) {
        state.grants.push({
          promotionId: promotionId,
          expiryTime: 0,
          probi: '',
          type: action.payload.properties.type
        })
      }

      state = {
        ...state,
        grants: state.grants
      }

      break
    case types.GET_GRANT_CAPTCHA:
      if (!state.grants) {
        break
      }

      const currentGrant = getGrant(action.payload.promotionId, state.grants)

      if (!currentGrant) {
        break
      }

      state.currentGrant = currentGrant
      chrome.send('brave_rewards.getGrantCaptcha', [])
      break
    case types.ON_GRANT_CAPTCHA:
      {
        if (state.currentGrant && state.grants) {
          let grant = state.currentGrant
          const props = action.payload.captcha
          grant.captcha = `data:image/jpeg;base64,${props.image}`
          grant.hint = props.hint

          const grants = state.grants.map((item: Rewards.Grant) => {
            let newGrant = item

            if (grant.promotionId === item.promotionId) {
              newGrant = Object.assign({
                captcha: grant.captcha,
                hint: props.hint,
              }, item)
            }

            return newGrant
          })

          state = {
            ...state,
            grants
          }
        }
        break
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
        if (state.currentGrant && state.grants) {
          let currentGrant: any = state.currentGrant

          const grants = state.grants.map((item: Rewards.Grant) => {
            if (currentGrant.promotionId === item.promotionId) {
              return {
                promotionId: currentGrant.promotionId,
                probi: '',
                expiryTime: 0,
                type: currentGrant.type
              }
            }
            return item
          })

          currentGrant = undefined

          state = {
            ...state,
            grants,
            currentGrant
          }
        }
        break
      }
    case types.ON_GRANT_DELETE:
      {
        if (state.currentGrant && state.grants) {
          let grants
          let grantIndex = -1
          let currentGrant: any = state.currentGrant

          state.grants.map((item: Rewards.Grant, i: number) => {
            if (currentGrant.promotionId === item.promotionId) {
              grantIndex = i
            }
          })

          if (grantIndex > -1) {
            grants = state.grants.splice(1, grantIndex)
            currentGrant = undefined
          }

          state = {
            ...state,
            grants,
            currentGrant
          }
        }
        break
      }
    case types.ON_GRANT_FINISH:
      {
        state = { ...state }
        let currentGrant: any = state.currentGrant
        const properties: Rewards.Grant = action.payload.properties

        if (!state.grants || !state.currentGrant) {
          break
        }

        switch (properties.status) {
          case 0:
            let ui = state.ui
            currentGrant.expiryTime = properties.expiryTime * 1000
            currentGrant.probi = properties.probi
            currentGrant.status = null
            ui.emptyWallet = false

            state = {
              ...state,
              ui
            }

            chrome.send('brave_rewards.getWalletProperties', [])
            break
          case 6:
            currentGrant.status = 'wrongPosition'
            chrome.send('brave_rewards.getGrantCaptcha', [])
            break
          case 13:
            currentGrant.status = 'grantGone'
            break
          case 18:
            currentGrant.status = 'grantAlreadyClaimed'
            break
          default:
            currentGrant.status = 'generalError'
            break
        }

        if (state.grants) {
          const grants = updateGrant(currentGrant, state.grants)

          state = {
            ...state,
            grants
          }
        }

        break
      }
  }

  return state
}

export default grantReducer
