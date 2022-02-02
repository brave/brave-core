/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ConnectWalletModal } from '../components/connect_wallet_modal'
import { ClaimGrantView } from '../components/claim_grant_view'
import { LocaleContext } from '../../shared/lib/locale_context'
import { WithThemeVariables } from '../../shared/components/with_theme_variables'
import { localeStrings } from './locale_strings'

export default {
  title: 'Rewards/Settings'
}

const locale = {
  getString (key: string) {
    return localeStrings[key] || 'MISSING'
  }
}

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

export function ConnectWallet () {
  const providers = [
    {
      type: 'uphold',
      name: 'Uphold'
    },
    {
      type: 'gemini',
      name: 'Gemini'
    }
  ]

  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <ConnectWalletModal
          rewardsBalance={10}
          providers={providers}
          onContinue={actionLogger('onContinue')}
          onClose={actionLogger('onClose')}
        />
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

export function Claim () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <div style={{ width: '366px' }}>
          <ClaimGrantView
            grantInfo={{
              id: 'grant-1',
              type: 'ads',
              amount: 3.25,
              createdAt: Date.now(),
              claimableUntil: Date.now() + 1000 * 60 * 60 * 24 * 5,
              expiresAt: Date.now() + 1000 * 60 * 60 * 24 * 5
            }}
            onClaim={actionLogger('onClaim')}
          />
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
