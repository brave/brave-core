/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { LoadingIcon } from '../../shared/components/icons/loading_icon'
import * as mojom from '../../shared/lib/mojom'

interface Props {
  info: RewardsInternals.Balance
  externalWallet: RewardsInternals.ExternalWallet
}

const getWalletName = (walletKey: string) => {
  switch (walletKey) {
    case 'blinded': {
      return getLocale('processorBraveTokens')
    }
    case 'uphold': {
      return getLocale('processorUphold')
    }
    case 'bitflyer': {
      return getLocale('processorBitflyer')
    }
    case 'gemini': {
      return getLocale('processorGemini')
    }
  }

  return 'Missing wallet'
}

const getWalletBalance = (externalWallet: RewardsInternals.ExternalWallet, wallets: Record<string, number>) => {
  const getBalanceForWallet = (wallet: string) => {
    const keys = Object.keys(wallets) as Array<string>
    return <div key={'wallet-' + wallet}>
      {getWalletName(wallet)}: {keys.includes(wallet) ? wallets[wallet] + ' ' + getLocale('bat') : <><LoadingIcon />{'Loading...'}</>}
    </div>
  }

  let items = []
  items.push(getBalanceForWallet('blinded'))
  if (externalWallet.status === mojom.WalletStatus.kConnected) {
    items.push(getBalanceForWallet(externalWallet.type))
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
      {getWalletBalance(props.externalWallet, props.info.wallets)}
    </>)
}
