// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { NavOption, WalletRoutes } from '../constants/types'

// Assets
import BuyIconURL from '../assets/svg-icons/buy-send-swap-deposit-icons/buy-icon.svg'
import SendIconURL from '../assets/svg-icons/buy-send-swap-deposit-icons/send-icon.svg'
import SwapIconURL from '../assets/svg-icons/buy-send-swap-deposit-icons/swap-icon.svg'
import DepositIconURL from '../assets/svg-icons/buy-send-swap-deposit-icons/deposit-icon.svg'
import HistoryIcon from '../assets/svg-icons/history-icon.svg'
import PortfolioIconURL from '../assets/svg-icons/buy-send-swap-deposit-icons/portfolio-icon.svg'

export const NavOptions: NavOption[] = [
  {
    id: 'portfolio',
    name: 'braveWalletTopNavPortfolio',
    icon: PortfolioIconURL,
    route: WalletRoutes.Portfolio
  },
  {
    id: 'buy',
    name: 'braveWalletBuy',
    icon: BuyIconURL,
    route: WalletRoutes.FundWalletPageStart
  },
  {
    id: 'send',
    name: 'braveWalletSend',
    icon: SendIconURL,
    route: WalletRoutes.Send
  },
  {
    id: 'swap',
    name: 'braveWalletSwap',
    icon: SwapIconURL,
    route: WalletRoutes.Swap
  },
  {
    id: 'deposit',
    name: 'braveWalletDepositCryptoButton',
    icon: DepositIconURL,
    route: WalletRoutes.DepositFundsPageStart
  },
  {
    id: 'transactions',
    name: 'braveWalletTransactions',
    icon: HistoryIcon,
    route: WalletRoutes.Accounts
  }
]
