import * as React from 'react'
import {
  BuySendSwapTypes,
  UserAccountType,
  ToOrFromType,
  BraveWallet,
  BuySupportedChains,
  DefaultCurrencies,
  AmountValidationErrorType
} from '../../constants/types'
import Swap from '../../components/buy-send-swap/tabs/swap-tab'
import Send from '../../components/buy-send-swap/tabs/send-tab'
import Buy from '../../components/buy-send-swap/tabs/buy-tab'
import {
  Layout
} from '../../components/buy-send-swap'
import { useSwap } from '../../common/hooks'

export interface Props {
  networkList: BraveWallet.NetworkInfo[]
  selectedSendAsset: BraveWallet.BlockchainToken | undefined
  sendAssetBalance: string
  selectedNetwork: BraveWallet.NetworkInfo
  selectedTab: BuySendSwapTypes
  sendAmountValidationError: AmountValidationErrorType | undefined
  buyAmount: string
  sendAmount: string
  toAddressOrUrl: string
  toAddress: string
  addressError: string
  addressWarning: string
  buyAssetOptions: BraveWallet.BlockchainToken[]
  sendAssetOptions: BraveWallet.BlockchainToken[]
  defaultCurrencies: DefaultCurrencies
  onSubmitBuy: (asset: BraveWallet.BlockchainToken) => void
  onSubmitSend: () => void
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => void
  onSelectAccount: (account: UserAccountType) => void
  onSetBuyAmount: (value: string) => void
  onSetSendAmount: (value: string) => void
  onSetToAddressOrUrl: (value: string) => void
  onSelectPresetSendAmount: (percent: number) => void
  onSelectTab: (tab: BuySendSwapTypes) => void
  onSelectSendAsset: (asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => void
  onAddNetwork: () => void
  onAddAsset: (value: boolean) => void
}

function BuySendSwap (props: Props) {
  const {
    networkList,
    selectedNetwork,
    selectedTab,
    buyAmount,
    sendAmount,
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAssetBalance,
    toAddress,
    toAddressOrUrl,
    buyAssetOptions,
    sendAssetOptions,
    sendAmountValidationError,
    defaultCurrencies,
    onSubmitBuy,
    onSubmitSend,
    onSelectNetwork,
    onSelectAccount,
    onSetBuyAmount,
    onSetSendAmount,
    onSetToAddressOrUrl,
    onSelectPresetSendAmount,
    onSelectTab,
    onSelectSendAsset,
    onAddNetwork,
    onAddAsset
  } = props

  const { isSwapSupported } = useSwap({})

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

  const onClickAddAsset = () => {
    onAddAsset(true)
  }

  return (
    <Layout
      selectedNetwork={selectedNetwork}
      isBuyDisabled={isBuyDisabled}
      isSwapDisabled={!isSwapSupported}
      selectedTab={selectedTab}
      onChangeTab={changeTab}
    >
      {selectedTab === 'swap' &&
        <Swap
          onSelectNetwork={onSelectNetwork}
          onSelectAccount={onSelectAccount}
          onAddNetwork={onAddNetwork}
          onAddAsset={onClickAddAsset}
        />
      }
      {selectedTab === 'send' &&
        <Send
          amountValidationError={sendAmountValidationError}
          addressError={addressError}
          addressWarning={addressWarning}
          selectedAssetAmount={sendAmount}
          selectedAssetBalance={sendAssetBalance}
          toAddressOrUrl={toAddressOrUrl}
          toAddress={toAddress}
          onSelectAccount={onSelectAccount}
          onSelectNetwork={onSelectNetwork}
          onSelectPresetAmount={onSelectPresetSendAmount}
          onSelectAsset={onSelectSendAsset}
          onSetSendAmount={onSetSendAmount}
          onSetToAddressOrUrl={onSetToAddressOrUrl}
          onSubmit={onSubmitSend}
          selectedNetwork={selectedNetwork}
          selectedAsset={selectedSendAsset}
          showHeader={true}
          assetOptions={sendAssetOptions}
          onAddNetwork={onAddNetwork}
          onAddAsset={onClickAddAsset}
        />
      }
      {selectedTab === 'buy' &&
        <Buy
          defaultCurrencies={defaultCurrencies}
          networkList={networkList}
          buyAmount={buyAmount}
          onSelectAccount={onSelectAccount}
          onSelectNetwork={onSelectNetwork}
          onSubmit={onSubmitBuy}
          onSetBuyAmount={onSetBuyAmount}
          selectedNetwork={selectedNetwork}
          showHeader={true}
          assetOptions={buyAssetOptions}
          onAddNetwork={onAddNetwork}
          onAddAsset={onClickAddAsset}
        />
      }
    </Layout>
  )
}

export default BuySendSwap
