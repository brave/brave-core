import * as React from 'react'
import { useSelector } from 'react-redux'
import {
  BuySendSwapTypes,
  UserAccountType,
  BraveWallet,
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
  buyAmount: string
  buyAssetOptions: BraveWallet.BlockchainToken[]
  onSubmitBuy: (asset: BraveWallet.BlockchainToken) => void
  onSelectAccount: (account: UserAccountType) => void
  onSetBuyAmount: (value: string) => void
  onSelectTab: (tab: BuySendSwapTypes) => void
  selectedBuyOption: BraveWallet.OnRampProvider
  onSelectBuyOption: (optionId: BraveWallet.OnRampProvider) => void
  wyreAssetOptions: BraveWallet.BlockchainToken[]
  rampAssetOptions: BraveWallet.BlockchainToken[]
}

function BuySendSwap (props: Props) {
  const {
    selectedTab,
    buyAmount,
    buyAssetOptions,
    onSubmitBuy,
    onSelectAccount,
    onSetBuyAmount,
    onSelectTab,
    selectedBuyOption,
    onSelectBuyOption,
    rampAssetOptions,
    wyreAssetOptions
  } = props

  // redux
  const {
    networkList,
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
              networkList={networkList}
              buyAmount={buyAmount}
              onSelectAccount={onSelectAccount}
              onSubmit={onSubmitBuy}
              onSetBuyAmount={onSetBuyAmount}
              selectedNetwork={selectedNetwork}
              showHeader={true}
              assetOptions={buyAssetOptions}
              wyreAssetOptions={wyreAssetOptions}
              rampAssetOptions={rampAssetOptions}
              selectedBuyOption={selectedBuyOption}
              onSelectBuyOption={onSelectBuyOption}
            />
          }
        </>
      )}
    </Layout>
  )
}

export default BuySendSwap
