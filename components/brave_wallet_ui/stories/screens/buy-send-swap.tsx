import * as React from 'react'
import { useSelector } from 'react-redux'
import {
  BuySendSwapTypes,
  UserAccountType,
  BuySupportedChains,
  WalletState
} from '../../constants/types'
import SwapTab from '../../components/buy-send-swap/tabs/swap-tab'
import SendTab from '../../components/buy-send-swap/tabs/send-tab'
import Buy from '../../components/buy-send-swap/tabs/buy-tab'
import {
  Layout,
  CreateAccountTab
} from '../../components/buy-send-swap'
import { useSwap, useHasAccount, usePrevNetwork } from '../../common/hooks'

export interface Props {
  selectedTab: BuySendSwapTypes
  onSelectAccount: (account: UserAccountType) => void
  onSelectTab: (tab: BuySendSwapTypes) => void
}

function BuySendSwap (props: Props) {
  const {
    selectedTab,
    onSelectAccount,
    onSelectTab
  } = props

  // redux
  const {
    selectedNetwork,
    defaultCurrencies
  } = useSelector((state: { wallet: WalletState }) => state.wallet)

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
  }, [selectedNetwork, selectedTab, BuySupportedChains, isSwapSupported])

  const isBuyDisabled = React.useMemo(() => {
    return !BuySupportedChains.includes(selectedNetwork.chainId)
  }, [BuySupportedChains, selectedNetwork])

  const changeTab = (tab: BuySendSwapTypes) => () => {
    onSelectTab(tab)
  }

  return (
    <Layout
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
    </Layout>
  )
}

export default BuySendSwap
