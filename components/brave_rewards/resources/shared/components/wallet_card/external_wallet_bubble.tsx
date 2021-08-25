/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  ExternalWallet,
  getExternalWalletProviderName
} from '../../lib/external_wallet'

import { ExternalWalletAction } from './external_wallet_action'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { GeminiIcon } from '../icons/gemini_icon'
import { BitflyerIcon } from '../icons/bitflyer_icon'
import { UpholdIcon } from '../icons/uphold_icon'
import { PendingIcon } from './icons/pending_icon'

import * as styles from './external_wallet_bubble.style'

interface Props {
  externalWallet: ExternalWallet
  onExternalWalletAction: (action: ExternalWalletAction) => void
  onCloseBubble: () => void
}

export function ExternalWalletBubble (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const { externalWallet } = props

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

  const providerName = getExternalWalletProviderName(externalWallet.provider)

  return (
      <styles.root>
        <styles.content>
          <styles.header>
            <styles.providerIcon>
              <ProviderIcon />
            </styles.providerIcon>
            <styles.username>
              {externalWallet.username}
            </styles.username>
            <styles.status className={externalWallet.status}>
              {externalWallet.status === 'pending' && <PendingIcon />}
              {getWalletStatus()}
            </styles.status>
          </styles.header>
          {
            externalWallet.status === 'pending' &&
              <styles.pendingNotice>
                <PendingIcon />
                <span>{getString('walletCompleteVerificationText')}</span>
              </styles.pendingNotice>
          }
          <styles.links>
            <styles.link>
              <styles.linkMarker />
              {
                externalWallet.status === 'pending'
                  ? <a href='#' onClick={actionHandler('complete-verification')}>
                      {
                        formatMessage(
                          getString('walletCompleteVerificationLink'),
                          [providerName])
                      }
                    </a>
                  : <a href='#' onClick={actionHandler('view-account')}>
                      {
                        formatMessage(
                          getString('walletAccountLink'),
                          [providerName])
                      }
                    </a>
              }
            </styles.link>
            <styles.link>
              <styles.linkMarker />
              <a href='#' onClick={actionHandler('disconnect')}>
                {getString('walletDisconnectLink')}
              </a>
            </styles.link>
          </styles.links>
        </styles.content>
        <styles.backdrop onClick={props.onCloseBubble} />
      </styles.root>
  )
}
