/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ExternalWallet } from '../../lib/external_wallet'
import { ExternalWalletAction } from './external_wallet_action'
import { LocaleContext } from '../../lib/locale_context'
import { GeminiIcon } from '../icons/gemini_icon'
import { BitflyerIcon } from '../icons/bitflyer_icon'
import { UpholdIcon } from '../icons/uphold_icon'
import { CaretIcon } from '../icons/caret_icon'
import { ArrowNextIcon } from '../icons/arrow_next_icon'
import { ExternalWalletBubble } from './external_wallet_bubble'

import * as style from './external_wallet_view.style'

import * as mojom from '../../../shared/lib/mojom'

interface Props {
  externalWallet: ExternalWallet | null
  onExternalWalletAction: (action: ExternalWalletAction) => void
}

export function ExternalWalletView (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [showBubble, setShowBubble] = React.useState(false)

  const { externalWallet } = props

  function actionHandler (action: ExternalWalletAction) {
    return () => props.onExternalWalletAction(action)
  }

  function ProviderIcon () {
    if (!externalWallet) {
      return null
    }

    switch (externalWallet.provider) {
      case 'gemini': return <GeminiIcon />
      case 'bitflyer': return <BitflyerIcon />
      case 'uphold': return <UpholdIcon />
    }
  }

  function toggleBubble () {
    setShowBubble(!showBubble)
  }

  function renderButton () {
    if (!externalWallet) {
      return (
        <style.verifyWallet>
          <button
            data-test-id='verify-rewards-button'
            className='connect'
            onClick={actionHandler('verify')}
          >
            <style.buttonText>
              {getString('walletUnverified')}
            </style.buttonText>
            <style.buttonIcons>
              <ArrowNextIcon />
            </style.buttonIcons>
          </button>
        </style.verifyWallet>
      )
    }

    return (
      <style.bubbleAction>
        <button onClick={toggleBubble} className={showBubble ? 'pressed' : ''}>
          <style.buttonText data-test-id='external-wallet-status-text'>
            {
              getString(externalWallet.status === mojom.WalletStatus.kLoggedOut
                ? 'walletDisconnected'
                : 'walletVerified')
            }
          </style.buttonText>
          <style.buttonIcons>
            <span className='provider'><ProviderIcon /></span>
            <span className='caret'>
              <CaretIcon direction='down' />
            </span>
          </style.buttonIcons>
        </button>
      </style.bubbleAction>
    )
  }

  function renderBubble () {
    if (!showBubble || !externalWallet) {
      return null
    }

    return (
      <ExternalWalletBubble
        externalWallet={externalWallet}
        onExternalWalletAction={props.onExternalWalletAction}
        onCloseBubble={toggleBubble}
      />
    )
  }

  return (
    <style.root>
      {renderButton()}
      {renderBubble()}
    </style.root>
  )
}
