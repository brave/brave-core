/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { ExternalWalletAction } from './external_wallet_action'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { WalletProviderIcon } from '../icons/wallet_provider_icon'

import * as style from './external_wallet_bubble.style'

import * as mojom from '../../../shared/lib/mojom'

interface Props {
  externalWallet: ExternalWallet
  onExternalWalletAction: (action: ExternalWalletAction) => void
  onCloseBubble: () => void
}

export function ExternalWalletBubble (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { externalWallet } = props
  const providerName = getExternalWalletProviderName(externalWallet.provider)

  function actionHandler (action: ExternalWalletAction) {
    return () => { props.onExternalWalletAction(action) }
  }

  function getWalletStatus () {
    switch (externalWallet.status) {
      case mojom.WalletStatus.kLoggedOut:
        return getString('walletDisconnected')
      case mojom.WalletStatus.kConnected:
        return getString('walletVerified')
    }

    return ''
  }

  function renderAccountLink () {
    switch (externalWallet.status) {
      case mojom.WalletStatus.kLoggedOut:
        return (
          <button onClick={actionHandler('reconnect')}>
            {
              formatMessage(getString('walletLogIntoYourAccount'), [
                providerName
              ])
            }
          </button>
        )
      case mojom.WalletStatus.kConnected:
        return (
          <button onClick={actionHandler('view-account')}>
            {formatMessage(getString('walletAccountLink'), [providerName])}
          </button>
        )
    }

    return null
  }

  return (
    <style.root>
      <style.content>
        <style.header>
          <style.providerIcon>
            <WalletProviderIcon provider={externalWallet.provider} />
          </style.providerIcon>
          <style.username>
            {externalWallet.username}
          </style.username>
          <style.status className={externalWallet.status === mojom.WalletStatus.kConnected ? 'connected' : ''}>
            {getWalletStatus()}
          </style.status>
        </style.header>
        <style.links>
          <style.link>
            <style.linkMarker />
            {renderAccountLink()}
          </style.link>
        </style.links>
      </style.content>
      <style.backdrop onClick={props.onCloseBubble} />
    </style.root>
  )
}
