/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ConnectWalletModal } from '../components/connect_wallet_modal'
import { ManageWalletModal } from '../components/manage_wallet_modal'
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

export function ManageWallet () {
  return (
    <LocaleContext.Provider value={locale}>
      <WithThemeVariables>
        <ManageWalletModal
          paymentID='7212410d-c20e-4ac5-9f87-ba84913eda6a'
          linkedDevices={[
            {
              paymentID: '61734561-0114-4d97-97c0-8529f85cd2fb',
              updating: false
            },
            {
              paymentID: '7212410d-c20e-4ac5-9f87-ba84913eda6a',
              updating: false
            }
          ]}
          externalWalletProvider={'uphold'}
          openDeviceSlots={2}
          unlinkingAvailableAt={Date.now() + 1000 * 60 * 60 * 24 * 68}
          unlinkingInterval={1000 * 60 * 60 * 24 * 90}
          tokenBalance={8.25}
          onClose={actionLogger('onClose')}
          onReset={actionLogger('onReset')}
          onRestoreWallet={actionLogger('onRestoreWallet')}
          onViewQR={actionLogger('onViewQR')}
          onVerifyWallet={actionLogger('onVerifyWallet')}
          onUnlinkDevice={actionLogger('onUnlinkDevice')}
        />
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}
