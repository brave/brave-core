/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
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
      return Object.assign(grant, newGrant)
    }
    return grant
  })
}

const grantTypeMap = {
  android: 'ugp'
}

const grantReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.GET_GRANTS:
      chrome.send('brave_rewards.getGrants', ['', ''])
      break
    case types.ON_GRANT:
      state = { ...state }
      const grantProps = action.payload.properties

      if (grantProps.status === 1) {
        break
      }

      if (grantProps.status === 13 && !grantProps.promotionId) {
        state.grants = []
        break
      }

      if (!state.grants) {
        state.grants = []
      }

      const promotionId = grantProps.promotionId

      const grantType = grantTypeMap[grantProps.type] || grantProps.type

      if (!getGrant(promotionId, state.grants)) {
        state.grants.push({
          promotionId: promotionId,
          expiryTime: 0,
          probi: '',
          type: grantType
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
      chrome.send('brave_rewards.getGrantCaptcha', [currentGrant.promotionId, currentGrant.type])
      break
    case types.ON_GRANT_CAPTCHA: {
      if (state.currentGrant && state.grants) {
        const props = action.payload.captcha
        let hint = props.hint
        let captcha = `data:image/jpeg;base64,${props.image}`

        const grants = state.grants.map((item: Rewards.Grant) => {
          let newGrant = item
          let promotionId = state.currentGrant && state.currentGrant.promotionId

          if (promotionId === item.promotionId) {
            newGrant = item
            newGrant.captcha = captcha
            newGrant.hint = hint
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
    case types.SOLVE_GRANT_CAPTCHA: {
      const promotionId = state.currentGrant && state.currentGrant.promotionId

      if (promotionId && action.payload.x && action.payload.y) {
        chrome.send('brave_rewards.solveGrantCaptcha', [JSON.stringify({
          x: action.payload.x,
          y: action.payload.y
        }), promotionId])
      }
      break
    }
    case types.ON_GRANT_RESET: {
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
    case types.ON_GRANT_DELETE: {
      if (state.currentGrant && state.grants) {
        let grantIndex = -1
        let currentGrant: any = state.currentGrant

        state.grants.map((item: Rewards.Grant, i: number) => {
          if (currentGrant.promotionId === item.promotionId) {
            grantIndex = i
          }
        })

        if (grantIndex > -1) {
          state.grants.splice(grantIndex, 1)
          currentGrant = undefined
        }

        state = {
          ...state,
          currentGrant
        }
      }
      break
    }
    case types.ON_GRANT_FINISH: {
      state = { ...state }
      let newGrant: any = {}
      const properties: Rewards.Grant = action.payload.properties
      const panelClaimed = properties.status === 0 && !state.currentGrant

      if (panelClaimed) {
        state.grants = []
        break
      }

      if (!state.grants || !state.currentGrant) {
        break
      }

      newGrant.promotionId = state.currentGrant.promotionId

      switch (properties.status) {
        case 0:
          let ui = state.ui
          newGrant.expiryTime = properties.expiryTime * 1000
          newGrant.probi = properties.probi
          newGrant.status = null
          ui.emptyWallet = false

          state = {
            ...state,
            ui
          }

          chrome.send('brave_rewards.getWalletProperties', [])
          chrome.send('brave_rewards.fetchBalance', [])
          break
        case 6:
          newGrant.status = 'wrongPosition'
          chrome.send('brave_rewards.getGrantCaptcha', [])
          break
        case 13:
          newGrant.status = 'grantGone'
          break
        case 18:
          newGrant.status = 'grantAlreadyClaimed'
          break
        case 19:
          state.safetyNetFailed = true
          break
        default:
          newGrant.status = 'generalError'
          break
      }

      if (state.grants) {
        const grants = updateGrant(newGrant, state.grants)

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
