/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'

const getGrant = (id?: string, grants?: RewardsExtension.GrantInfo[]) => {
  if (!id || !grants) {
    return null
  }

  return grants.find((grant: RewardsExtension.GrantInfo) => {
    return (grant.promotionId === id)
  })
}

const updateGrant = (newGrant: RewardsExtension.GrantInfo, grants: RewardsExtension.GrantInfo[]) => {
  return grants.map((grant: RewardsExtension.GrantInfo) => {
    if (newGrant.promotionId === grant.promotionId) {
      return Object.assign(newGrant, grant)
    }
    return grant
  })
}

export const grantPanelReducer = (state: RewardsExtension.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  switch (action.type) {
    case types.GET_GRANTS:
      chrome.braveRewards.getGrants()
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
      chrome.braveRewards.getGrantCaptcha()
      break
    case types.ON_GRANT_CAPTCHA:
      {
        if (state.currentGrant && state.grants) {
          const props = action.payload.captcha
          let hint = props.hint
          let captcha = `data:image/jpeg;base64,${props.image}`

          const grants = state.grants.map((item: RewardsExtension.GrantInfo) => {
            let newGrant = item
            let promotionId = state.currentGrant && state.currentGrant.promotionId

            if (promotionId === item.promotionId) {
              newGrant = Object.assign({
                captcha: captcha,
                hint: hint
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
      {
        const promotionId = state.currentGrant && state.currentGrant.promotionId

        if (promotionId && action.payload.x && action.payload.y) {
          chrome.braveRewards.solveGrantCaptcha(JSON.stringify({
            x: action.payload.x,
            y: action.payload.y
          }), promotionId)
        }
        break
      }
    case types.ON_GRANT_RESET:
      {
        if (state.currentGrant && state.grants) {
          let currentGrant: any = state.currentGrant

          const grants = state.grants.map((item: RewardsExtension.GrantInfo) => {
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
          let grantIndex = -1
          let currentGrant: any = state.currentGrant

          state.grants.map((item: RewardsExtension.GrantInfo, i: number) => {
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
    case types.ON_GRANT_FINISH:
      {
        state = { ...state }
        let currentGrant: any = state.currentGrant
        const properties: RewardsExtension.GrantInfo = action.payload.properties

        if (!state.grants || !state.currentGrant) {
          break
        }

        switch (properties.status) {
          case 0:
            currentGrant.expiryTime = properties.expiryTime * 1000
            currentGrant.probi = properties.probi
            currentGrant.status = null
            chrome.braveRewards.getWalletProperties()
            break
          case 6:
            currentGrant.status = 'wrongPosition'
            chrome.braveRewards.getGrantCaptcha()
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
