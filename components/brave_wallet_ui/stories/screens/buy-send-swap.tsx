import * as React from 'react'
import {
  BuySendSwapTypes,
  UserAccountType,
  OrderTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  BraveWallet,
  BuySupportedChains,
  SwapValidationErrorType,
  DefaultCurrencies,
  AmountValidationErrorType
} from '../../constants/types'
import Swap from '../../components/buy-send-swap/tabs/swap-tab'
import Send from '../../components/buy-send-swap/tabs/send-tab'
import Buy from '../../components/buy-send-swap/tabs/buy-tab'
import {
  Layout
} from '../../components/buy-send-swap'

export interface Props {
  accounts: UserAccountType[]
  networkList: BraveWallet.EthereumChain[]
  orderType: OrderTypes
  selectedSendAsset: BraveWallet.BlockchainToken
  sendAssetBalance: string
  swapToAsset: BraveWallet.BlockchainToken
  swapFromAsset: BraveWallet.BlockchainToken
  selectedNetwork: BraveWallet.EthereumChain
  selectedAccount: UserAccountType
  selectedTab: BuySendSwapTypes
  exchangeRate: string
  slippageTolerance: SlippagePresetObjectType
  swapValidationError?: SwapValidationErrorType
  sendAmountValidationError?: AmountValidationErrorType
  orderExpiration: ExpirationPresetObjectType
  buyAmount: string
  sendAmount: string
  fromAmount: string
  toAmount: string
  fromAssetBalance: string
  toAssetBalance: string
  toAddressOrUrl: string
  toAddress: string
  addressError: string
  addressWarning: string
  buyAssetOptions: BraveWallet.BlockchainToken[]
  sendAssetOptions: BraveWallet.BlockchainToken[]
  swapAssetOptions: BraveWallet.BlockchainToken[]
  isFetchingSwapQuote: boolean
  isSwapSubmitDisabled: boolean
  isSwapSupported: boolean
  customSlippageTolerance: string
  defaultCurrencies: DefaultCurrencies
  onCustomSlippageToleranceChange: (value: string) => void
  onSubmitBuy: (asset: BraveWallet.BlockchainToken) => void
  onSubmitSend: () => void
  onSubmitSwap: () => void
  flipSwapAssets: () => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onSelectAccount: (account: UserAccountType) => void
  onToggleOrderType: () => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => void
  onSelectSlippageTolerance: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration: (expiration: ExpirationPresetObjectType) => void
  onSetExchangeRate: (value: string) => void
  onSetBuyAmount: (value: string) => void
  onSetSendAmount: (value: string) => void
  onSetFromAmount: (value: string) => void
  onSetToAddressOrUrl: (value: string) => void
  onSetToAmount: (value: string) => void
  onSelectPresetFromAmount: (percent: number) => void
  onSelectPresetSendAmount: (percent: number) => void
  onSelectTab: (tab: BuySendSwapTypes) => void
  onSwapQuoteRefresh: () => void
  onSelectSendAsset: (asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => void
  onAddNetwork: () => void
  onAddAsset: (value: boolean) => void
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
    selectedTab,
    exchangeRate,
    slippageTolerance,
    orderExpiration,
    buyAmount,
    sendAmount,
    fromAmount,
    toAmount,
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAssetBalance,
    fromAssetBalance,
    toAssetBalance,
    toAddress,
    toAddressOrUrl,
    buyAssetOptions,
    sendAssetOptions,
    swapAssetOptions,
    swapValidationError,
    sendAmountValidationError,
    isFetchingSwapQuote,
    isSwapSubmitDisabled,
    isSwapSupported,
    customSlippageTolerance,
    defaultCurrencies,
    onCustomSlippageToleranceChange,
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
    onSetToAddressOrUrl,
    onSetToAmount,
    onSelectPresetFromAmount,
    onSelectPresetSendAmount,
    onSelectTab,
    onSwapQuoteRefresh,
    onSelectSendAsset,
    onAddNetwork,
    onAddAsset
  } = props

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
          isFetchingQuote={isFetchingSwapQuote}
          isSubmitDisabled={isSwapSubmitDisabled}
          validationError={swapValidationError}
          customSlippageTolerance={customSlippageTolerance}
          onCustomSlippageToleranceChange={onCustomSlippageToleranceChange}
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
          onQuoteRefresh={onSwapQuoteRefresh}
          onAddNetwork={onAddNetwork}
          onAddAsset={onClickAddAsset}
        />
      }
      {selectedTab === 'send' &&
        <Send
          amountValidationError={sendAmountValidationError}
          addressError={addressError}
          addressWarning={addressWarning}
          accounts={accounts}
          networkList={networkList}
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
          selectedAccount={selectedAccount}
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
          onAddNetwork={onAddNetwork}
          onAddAsset={onClickAddAsset}
        />
      }
    </Layout>
  )
}

export default BuySendSwap
