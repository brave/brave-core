import * as React from 'react'
import {
  BuySendSwapTypes,
  UserAccountType,
  AccountAssetOptionType,
  OrderTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  EthereumChain
} from '../../constants/types'
import Swap from '../../components/buy-send-swap/tabs/swap-tab'
import Send from '../../components/buy-send-swap/tabs/send-tab'
import Buy from '../../components/buy-send-swap/tabs/buy-tab'
import {
  Layout
} from '../../components/buy-send-swap'

export interface Props {
  accounts: UserAccountType[]
  networkList: EthereumChain[]
  orderType: OrderTypes
  swapToAsset: AccountAssetOptionType
  swapFromAsset: AccountAssetOptionType
  selectedNetwork: EthereumChain
  selectedAccount: UserAccountType
  exchangeRate: string
  slippageTolerance: SlippagePresetObjectType
  orderExpiration: ExpirationPresetObjectType
  buyAmount: string
  sendAmount: string
  fromAmount: string
  toAmount: string
  fromAssetBalance: string
  toAssetBalance: string
  toAddress: string
  buyAssetOptions: AccountAssetOptionType[]
  sendAssetOptions: AccountAssetOptionType[]
  swapAssetOptions: AccountAssetOptionType[]
  onSubmitBuy: (asset: AccountAssetOptionType) => void
  onSubmitSend: () => void
  onSubmitSwap: () => void
  flipSwapAssets: () => void
  onSelectNetwork: (network: EthereumChain) => void
  onSelectAccount: (account: UserAccountType) => void
  onToggleOrderType: () => void
  onSelectAsset: (asset: AccountAssetOptionType, toOrFrom: ToOrFromType) => void
  onSelectSlippageTolerance: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration: (expiration: ExpirationPresetObjectType) => void
  onSetExchangeRate: (value: string) => void
  onSetBuyAmount: (value: string) => void
  onSetSendAmount: (value: string) => void
  onSetFromAmount: (value: string) => void
  onSetToAddress: (value: string) => void
  onSetToAmount: (value: string) => void
  onSelectPresetFromAmount: (percent: number) => void
  onSelectPresetSendAmount: (percent: number) => void
}

function BuySendSwap (props: Props) {
  const {
    accounts,
    networkList,
    orderType,
    swapToAsset,
    swapFromAsset,
    selectedNetwork,
    selectedAccount,
    exchangeRate,
    slippageTolerance,
    orderExpiration,
    buyAmount,
    sendAmount,
    fromAmount,
    toAmount,
    fromAssetBalance,
    toAssetBalance,
    toAddress,
    buyAssetOptions,
    sendAssetOptions,
    swapAssetOptions,
    onSubmitBuy,
    onSubmitSend,
    onSubmitSwap,
    flipSwapAssets,
    onSelectNetwork,
    onSelectAccount,
    onToggleOrderType,
    onSelectAsset,
    onSelectSlippageTolerance,
    onSelectExpiration,
    onSetExchangeRate,
    onSetBuyAmount,
    onSetSendAmount,
    onSetFromAmount,
    onSetToAddress,
    onSetToAmount,
    onSelectPresetFromAmount,
    onSelectPresetSendAmount
  } = props
  const [selectedTab, setSelectedTab] = React.useState<BuySendSwapTypes>('buy')

  const changeTab = (tab: BuySendSwapTypes) => () => {
    setSelectedTab(tab)
  }

  return (
    <Layout selectedTab={selectedTab} onChangeTab={changeTab}>
      {selectedTab === 'swap' &&
        <Swap
          accounts={accounts}
          networkList={networkList}
          orderType={orderType}
          swapToAsset={swapToAsset}
          swapFromAsset={swapFromAsset}
          selectedNetwork={selectedNetwork}
          selectedAccount={selectedAccount}
          exchangeRate={exchangeRate}
          orderExpiration={orderExpiration}
          slippageTolerance={slippageTolerance}
          fromAmount={fromAmount}
          toAmount={toAmount}
          fromAssetBalance={fromAssetBalance}
          toAssetBalance={toAssetBalance}
          onSubmitSwap={onSubmitSwap}
          flipSwapAssets={flipSwapAssets}
          onSelectNetwork={onSelectNetwork}
          onSelectAccount={onSelectAccount}
          onSelectSwapAsset={onSelectAsset}
          onToggleOrderType={onToggleOrderType}
          onSelectSlippageTolerance={onSelectSlippageTolerance}
          onSelectExpiration={onSelectExpiration}
          onSetExchangeRate={onSetExchangeRate}
          onSetFromAmount={onSetFromAmount}
          onSetToAmount={onSetToAmount}
          onSelectPresetAmount={onSelectPresetFromAmount}
          assetOptions={swapAssetOptions}
        />
      }
      {selectedTab === 'send' &&
        <Send
          accounts={accounts}
          networkList={networkList}
          selectedAssetAmount={sendAmount}
          selectedAssetBalance={fromAssetBalance}
          toAddress={toAddress}
          onSelectAccount={onSelectAccount}
          onSelectNetwork={onSelectNetwork}
          onSelectPresetAmount={onSelectPresetSendAmount}
          onSelectAsset={onSelectAsset}
          onSetSendAmount={onSetSendAmount}
          onSetToAddress={onSetToAddress}
          onSubmit={onSubmitSend}
          selectedAccount={selectedAccount}
          selectedNetwork={selectedNetwork}
          selectedAsset={swapFromAsset}
          showHeader={true}
          assetOptions={sendAssetOptions}
        />
      }
      {selectedTab === 'buy' &&
        <Buy
          accounts={accounts}
          networkList={networkList}
          buyAmount={buyAmount}
          onSelectAccount={onSelectAccount}
          onSelectNetwork={onSelectNetwork}
          onSubmit={onSubmitBuy}
          onSetBuyAmount={onSetBuyAmount}
          selectedAccount={selectedAccount}
          selectedNetwork={selectedNetwork}
          showHeader={true}
          assetOptions={buyAssetOptions}
        />
      }
    </Layout>
  )
}

export default BuySendSwap
