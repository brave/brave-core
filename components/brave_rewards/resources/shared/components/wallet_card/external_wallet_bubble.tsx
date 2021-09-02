/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ExternalWallet, getExternalWalletProviderName } from '../../lib/external_wallet'
import { ExternalWalletAction } from './external_wallet_action'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GeminiIcon } from '../icons/gemini_icon'
import { BitflyerIcon } from '../icons/bitflyer_icon'
import { UpholdIcon } from '../icons/uphold_icon'
import { PendingIcon } from './icons/pending_icon'

import * as style from './external_wallet_bubble.style'

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
    return (evt: React.UIEvent) => {
      evt.preventDefault()
      props.onExternalWalletAction(action)
    }
  }

  function ProviderIcon () {
    switch (externalWallet.provider) {
      case 'gemini': return <GeminiIcon />
      case 'bitflyer': return <BitflyerIcon />
      case 'uphold': return <UpholdIcon />
    }
  }

  function getWalletStatus () {
    switch (externalWallet.status) {
      case 'disconnected': return getString('walletDisconnected')
      case 'pending': return getString('walletPending')
      case 'verified': return getString('walletVerified')
    }
  }

  function renderAccountLink () {
    switch (externalWallet.status) {
      case 'pending':
        return (
          <a href='#' onClick={actionHandler('complete-verification')}>
            {
              formatMessage(getString('walletCompleteVerificationLink'), [
                providerName
              ])
            }
          </a>
        )
      case 'disconnected':
        return (
          <a href='#' onClick={actionHandler('reconnect')}>
            {
              formatMessage(getString('walletLogIntoYourAccount'), [
                providerName
              ])
            }
          </a>
        )
      case 'verified':
        return (
          <a href='#' onClick={actionHandler('view-account')}>
            {formatMessage(getString('walletAccountLink'), [providerName])}
          </a>
        )
    }
  }

  return (
    <style.root>
      <style.content>
        <style.header>
          <style.providerIcon>
            <ProviderIcon />
          </style.providerIcon>
          <style.username>
            {externalWallet.username}
          </style.username>
          <style.status className={externalWallet.status}>
            {externalWallet.status === 'pending' && <PendingIcon />}
            {getWalletStatus()}
          </style.status>
        </style.header>
        {
          externalWallet.status === 'pending' &&
            <style.pendingNotice>
              <PendingIcon />
              <span>{getString('walletCompleteVerificationText')}</span>
            </style.pendingNotice>
        }
        <style.links>
          <style.link>
            <style.linkMarker />
            {renderAccountLink()}
          </style.link>
          <style.link>
            <style.linkMarker />
            <a href='#' onClick={actionHandler('disconnect')}>
              {getString('walletDisconnectLink')}
            </a>
          </style.link>
        </style.links>
      </style.content>
      <style.backdrop onClick={props.onCloseBubble} />
    </style.root>
  )
}
