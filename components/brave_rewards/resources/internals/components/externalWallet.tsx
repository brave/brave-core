/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { lookupExternalWalletProviderName } from '../../shared/lib/external_wallet'

import * as mojom from '../../shared/lib/mojom'

interface Props {
  info: RewardsInternals.ExternalWallet
}

const getWalletStatus = (status: mojom.WalletStatus) => {
  switch (status) {
    case mojom.WalletStatus.kNotConnected:
      return getLocale('walletStatusNotConnected')
    case mojom.WalletStatus.kConnected:
      return getLocale('walletStatusVerified')
    case mojom.WalletStatus.kLoggedOut:
      return getLocale('walletStatusDisconnectedVerified')
  }

  return getLocale('walletNotCreated')
}
export const ExternalWallet = (props: Props) => {
  if (!props.info) {
    return null
  }

  return (
    <>
      <h3>{getLocale('externalWallet')}</h3>
      <div>
        {getLocale('walletStatus')} {getWalletStatus(props.info.status)}
      </div>
      {
        props.info.address && props.info.address.length > 0
        ? <div>
          {getLocale('walletAddress')} {props.info.address}
        </div>
        : null
      }
      {
        props.info.status !== 0 && props.info.type && props.info.type.length > 0
        ? <div>
          {getLocale('custodian')} {lookupExternalWalletProviderName(props.info.type)}
        </div>
        : null
      }
      {
        props.info.status !== 0 && props.info.memberId && props.info.memberId.length > 0
        ? <div>
          {getLocale('custodianMemberId')} {props.info.memberId}
        </div>
        : null
      }
    </>)
}
