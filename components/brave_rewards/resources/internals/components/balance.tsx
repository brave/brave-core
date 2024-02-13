/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'
import { LoadingIcon } from '../../shared/components/icons/loading_icon'
import * as mojom from '../../shared/lib/mojom'

interface Props {
  balance: mojom.Balance
  externalWallet?: RewardsInternals.ExternalWallet
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
    case 'zebpay': {
      return getLocale('processorZebPay')
    }
    case 'solana': {
      return getLocale('processorSolana')
    }
  }

  return 'Missing wallet'
}

const getBalances = (
  balances: Record<string, number>,
  externalWallet?: RewardsInternals.ExternalWallet
) => {
  const walletKeys = Object.keys(balances)

  const getBalance = (wallet: string) => {
    return <div key={'wallet-' + wallet}>
      {getWalletName(wallet)}: {walletKeys.includes(wallet)
        ? balances[wallet] + ' ' + getLocale('bat')
        : <><LoadingIcon />{getLocale('loading')}</>}
    </div>
  }

  let items = []
  items.push(getBalance('blinded'))
  if (externalWallet?.status === mojom.WalletStatus.kConnected) {
    items.push(getBalance(externalWallet.type))
  }

  return items
}

export const Balance = (props: Props) => {
  return (
    <>
      <h3>{getLocale('balanceInfo')}</h3>
      <div>
        {getLocale('totalBalance')} {props.balance.total} {getLocale('bat')}
      </div>
      {getBalances(props.balance.wallets, props.externalWallet)}
    </>)
}
