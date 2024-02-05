/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { lookupExternalWalletProviderName } from '../../shared/lib/external_wallet'

import * as mojom from '../../shared/lib/mojom'

interface Props {
  info?: RewardsInternals.ExternalWallet
}

const getWalletStatus = (info?: RewardsInternals.ExternalWallet) => {
  if (!info) {
    return getLocale('walletStatusNotConnected')
  }

  switch (info.status) {
    case mojom.WalletStatus.kNotConnected:
      return getLocale('walletStatusNotConnected')
    case mojom.WalletStatus.kConnected:
      return getLocale('walletStatusConnected')
    case mojom.WalletStatus.kLoggedOut:
      return getLocale('walletStatusDisconnected')
  }

  return ''
}
export const ExternalWallet = (props: Props) => {
  return (
    <>
      <h3>{getLocale('externalWallet')}</h3>
      <div>
        {getLocale('walletStatus')} {getWalletStatus(props.info)}
      </div>
      {
        props.info &&
          <div>
            {getLocale('walletAddress')} {props.info.address}
          </div>
      }
      {
        props.info &&
          <div>
            {getLocale('externalWalletType')} {lookupExternalWalletProviderName(props.info.type)}
          </div>
      }
      {
        props.info &&
          <div>
            {getLocale('externalWalletAccount')} {props.info.memberId || props.info.address}
          </div>
      }
    </>)
}
