/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  info: RewardsInternals.Balance
}

const getWalletName = (walletKey: string) => {
  switch (walletKey) {
    case 'anonymous': {
      return getLocale('processorBraveUserFunds')
    }
    case 'blinded': {
      return getLocale('processorBraveTokens')
    }
    case 'uphold': {
      return getLocale('processorUphold')
    }
    case 'bitflyer': {
      return getLocale('processorBitflyer')
    }
  }

  return 'Missing wallet'
}

const getWalletBalance = (wallets: Record<string, number>) => {
  let items = []
  for (const key in wallets) {
    items.push(<div key={'wallet-' + key}> {getWalletName(key)}: {wallets[key]} {getLocale('bat')} </div>)
  }

  return items
}

export const Balance = (props: Props) => {
  if (!props.info) {
    return null
  }

  return (
    <>
      <h3>{getLocale('balanceInfo')}</h3>
      <div>
        {getLocale('totalBalance')} {props.info.total} {getLocale('bat')}
      </div>
      {getWalletBalance(props.info.wallets)}
    </>)
}
