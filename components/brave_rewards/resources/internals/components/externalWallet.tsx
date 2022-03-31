/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { getWalletProviderName } from '../../page/utils'

interface Props {
  info: RewardsInternals.ExternalWallet
}

const getWalletStatus = (status: RewardsInternals.WalletStatus) => {
  switch (status) {
    case 0: {
      return getLocale('walletStatusNotConnected')
    }
    case 1: {
      return getLocale('walletStatusConnected')
    }
    case 2: {
      return getLocale('walletStatusVerified')
    }
    case 3: {
      return getLocale('walletStatusDisconnectedNotVerified')
    }
    case 4: {
      return getLocale('walletStatusDisconnectedVerified')
    }
    case 5: {
      return getLocale('walletStatusPending')
    }
  }

  return 'No wallet'
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
          {getLocale('custodian')} {getWalletProviderName(props.info.type)}
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
