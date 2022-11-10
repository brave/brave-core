/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { ClaimGrantView } from './claim_grant_view'

function promotionTypesToGrantType (type: Rewards.PromotionTypes) {
  switch (type) {
    case 1: return 'ads' // Rewards.PromotionTypes.ADS
    default: return 'ugp'
  }
}

export function GrantList () {
  const actions = useActions()
  const { promotions } = useRewardsData((data) => ({
    promotions: data.promotions
  }))

  if (promotions.length === 0) {
    return null
  }

  return (
    <>
      {
        promotions.map((promotion) => {
          if (!promotion.promotionId) {
            return null
          }

          const onClaim = () => {
            actions.claimPromotion(promotion.promotionId)
          }

          return (
            <div key={promotion.promotionId} data-test-id='promotion-claim-box'>
              <ClaimGrantView
                grantInfo={{
                  id: promotion.promotionId,
                  type: promotionTypesToGrantType(promotion.type),
                  amount: promotion.amount,
                  createdAt: promotion.createdAt,
                  claimableUntil: promotion.claimableUntil,
                  expiresAt: promotion.expiresAt
                }}
                showSpinner={promotion.captchaStatus === 'start'}
                onClaim={onClaim}
              />
            </div>
          )
        })
      }
    </>
  )
}
