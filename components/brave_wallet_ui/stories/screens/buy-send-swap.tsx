// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// types
import {
  BuySendSwapTypes,
  UserAccountType,
  BuySupportedChains,
  WalletState
} from '../../constants/types'

// hooks
import { useSwap, useHasAccount, usePrevNetwork } from '../../common/hooks'

// components
import SwapTab from '../../components/buy-send-swap/tabs/swap-tab'
import SendTab from '../../components/buy-send-swap/tabs/send-tab'
import Buy from '../../components/buy-send-swap/tabs/buy-tab'
import { CreateAccountTab } from '../../components/buy-send-swap/create-account/index'
import { BuySendSwapLayout } from '../../components/buy-send-swap/buy-send-swap-layout/buy-send-swap-layout'

export interface Props {
  selectedTab: BuySendSwapTypes
  onSelectAccount: (account: UserAccountType) => void
  onSelectTab: (tab: BuySendSwapTypes) => void
}

export const BuySendSwap = ({
  selectedTab,
  onSelectAccount,
  onSelectTab
}: Props) => {
  // redux
  const selectedNetwork = useSelector(({ wallet }: { wallet: WalletState }) => wallet.selectedNetwork)
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)

  // custom hooks
  const { isSwapSupported } = useSwap({})
  const { needsAccount } = useHasAccount()
  const { prevNetwork } = usePrevNetwork()

  // Switched this to useLayoutEffect to fix bad setState call error
  // that was accouring when you would switch to a network that doesn't
  // support swap and buy.
  React.useLayoutEffect(() => {
    if (selectedTab === 'buy' && !BuySupportedChains.includes(selectedNetwork.chainId)) {
      onSelectTab('send')
    }
    if (selectedTab === 'swap' && !isSwapSupported) {
      onSelectTab('send')
    }
  }, [
    selectedNetwork.chainId,
    selectedTab,
    isSwapSupported,
    onSelectTab
  ])

  const isBuyDisabled = React.useMemo(() => {
    return !BuySupportedChains.includes(selectedNetwork.chainId)
  }, [BuySupportedChains, selectedNetwork.chainId])

  const changeTab = (tab: BuySendSwapTypes) => () => {
    onSelectTab(tab)
  }

  return (
    <BuySendSwapLayout
      selectedNetwork={selectedNetwork}
      isBuyDisabled={isBuyDisabled}
      isSwapDisabled={!isSwapSupported}
      selectedTab={selectedTab}
      onChangeTab={changeTab}
    >
      {needsAccount ? (
        <CreateAccountTab
          prevNetwork={prevNetwork}
        />
      ) : (
        <>
          {selectedTab === 'swap' &&
            <SwapTab />
          }
          {selectedTab === 'send' &&
            <SendTab
              showHeader={true}
            />
          }
          {selectedTab === 'buy' &&
            <Buy
              defaultCurrencies={defaultCurrencies}
              onSelectAccount={onSelectAccount}
              showHeader={true}
            />
          }
        </>
      )}
    </BuySendSwapLayout>
  )
}

export default BuySendSwap
