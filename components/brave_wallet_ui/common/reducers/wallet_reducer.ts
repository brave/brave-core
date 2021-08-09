/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import {
  WalletAccountType,
  WalletState,
  Network,
  GetAllTokensReturnInfo,
  TokenInfo,
  GetBalanceReturnInfo,
  GetERC20TokenBalanceReturnInfo
} from '../../constants/types'
import * as WalletActions from '../actions/wallet_actions'
import { InitializedPayloadType } from '../constants/action_types'
import { formatFiatBalance } from '../../utils/format-balances'

const defaultState: WalletState = {
  hasInitialized: false,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  selectedAccount: {} as WalletAccountType,
  selectedNetwork: Network.Mainnet,
  accounts: [],
  walletAccountNames: [],
  userVisibleTokens: [],
  userVisibleTokensInfo: [],
  transactions: [],
  fullTokenList: []
}

const reducer = createReducer<WalletState>({}, defaultState)

reducer.on(WalletActions.initialized, (state: any, payload: InitializedPayloadType) => {
  const accounts = payload.accounts.map((address: string, idx: number) => {
    return {
      id: `${idx + 1}`,
      name: payload.walletAccountNames[idx],
      address,
      balance: '0',
      fiatBalance: '0',
      asset: 'eth',
      accountType: 'Primary',
      tokens: []
    }
  })
  // VisibleTokens needs to be persited in prefs and returned in
  // in the initialized payload to be set here.
  return {
    ...state,
    hasInitialized: true,
    isWalletCreated: payload.isWalletCreated,
    isWalletLocked: payload.isWalletLocked,
    favoriteApps: payload.favoriteApps,
    accounts,
    isWalletBackedUp: payload.isWalletBackedUp,
    walletAccountNames: payload.walletAccountNames,
    selectedAccount: accounts[0],
    userVisibleTokens: ['eth', '0x0D8775F648430679A709E98d2b0Cb6250d2887EF']
  }
})

reducer.on(WalletActions.hasIncorrectPassword, (state: any, payload: boolean) => {
  return {
    ...state,
    hasIncorrectPassword: payload
  }
})

reducer.on(WalletActions.selectAccount, (state: any, payload: WalletAccountType) => {
  return {
    ...state,
    selectedAccount: payload
  }
})

reducer.on(WalletActions.setNetwork, (state: any, payload: Network) => {
  return {
    ...state,
    selectedNetwork: payload
  }
})

reducer.on(WalletActions.setVisibleTokensInfo, (state: any, payload: TokenInfo[]) => {
  const eth = {
    contractAddress: 'eth',
    name: 'Ethereum',
    isErc20: true,
    isErc721: false,
    symbol: 'ETH',
    decimals: 18,
    icon: ''
  }
  const list = [eth, ...payload]
  return {
    ...state,
    userVisibleTokensInfo: list
  }
})

reducer.on(WalletActions.setVisibleTokens, (state: any, payload: string[]) => {
  return {
    ...state,
    userVisibleTokens: payload
  }
})

reducer.on(WalletActions.setAllTokensList, (state: any, payload: GetAllTokensReturnInfo) => {
  return {
    ...state,
    fullTokenList: payload.tokens
  }
})

reducer.on(WalletActions.ethBalancesUpdated, (state: any, payload: GetBalanceReturnInfo[]) => {
  let accounts: WalletAccountType[] = [...state.accounts]

  accounts.forEach((account, index) => {
    if (payload[index].success) {
      accounts[index].balance = payload[index].balance
      accounts[index].fiatBalance = formatFiatBalance(payload[index].balance, 18, '2000').toString()  // TODO: use actual price info
    }
  })

  return {
    ...state,
    accounts
  }
})

reducer.on(WalletActions.tokenBalancesUpdated, (state: any, payload: GetERC20TokenBalanceReturnInfo[][]) => {
  const userVisibleTokensInfo: TokenInfo[] = state.userVisibleTokensInfo
  let accounts: WalletAccountType[] = [...state.accounts]

  accounts.forEach((account, accountIndex) => {
    payload[accountIndex].forEach((info, tokenIndex) => {
      let assetBalance = '0'
      let fiatBalance = '0'

      if (userVisibleTokensInfo[tokenIndex].contractAddress === 'eth') {
        assetBalance = account.balance
        fiatBalance = account.fiatBalance
      } else if (info.success) {
        assetBalance = info.balance
        fiatBalance = formatFiatBalance(info.balance, userVisibleTokensInfo[tokenIndex].decimals, '0.5') // TODO: compute real value using price info
      } else if (account.tokens[tokenIndex]) {
        assetBalance = account.tokens[tokenIndex].assetBalance
        fiatBalance = account.tokens[tokenIndex].fiatBalance
      }
      account.tokens.push({
        asset: userVisibleTokensInfo[tokenIndex],
        assetBalance,
        fiatBalance
      })
    })
  })
  return {
    ...state,
    accounts
  }
})

export default reducer
