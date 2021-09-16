import * as React from 'react'
import {
  UserAccountType,
  AccountAssetOptionType,
  OrderTypes,
  BuySendSwapViewTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  EthereumChain
} from '../../../constants/types'
import {
  AccountsAssetsNetworks,
  Header,
  Swap
} from '..'

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
  fromAmount: string
  toAmount: string
  fromAssetBalance: string
  toAssetBalance: string
  assetOptions: AccountAssetOptionType[]
  onSubmitSwap: () => void
  flipSwapAssets: () => void
  onSelectNetwork: (network: EthereumChain) => void
  onSelectAccount: (account: UserAccountType) => void
  onToggleOrderType: () => void
  onSelectSwapAsset: (asset: AccountAssetOptionType, toOrFrom: ToOrFromType) => void
  onSelectSlippageTolerance: (slippage: SlippagePresetObjectType) => void
  onSelectExpiration: (expiration: ExpirationPresetObjectType) => void
  onSetExchangeRate: (value: string) => void
  onSetFromAmount: (value: string) => void
  onSetToAmount: (value: string) => void
  onSelectPresetAmount: (percent: number) => void
}

function SwapTab (props: Props) {
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
    fromAmount,
    toAmount,
    fromAssetBalance,
    toAssetBalance,
    assetOptions,
    onSubmitSwap,
    flipSwapAssets,
    onSelectNetwork,
    onSelectAccount,
    onToggleOrderType,
    onSelectSwapAsset,
    onSelectSlippageTolerance,
    onSelectExpiration,
    onSetExchangeRate,
    onSetFromAmount,
    onSetToAmount,
    onSelectPresetAmount
  } = props
  const [swapView, setSwapView] = React.useState<BuySendSwapViewTypes>('swap')
  const [isSelectingAsset, setIsSelectingAsset] = React.useState<ToOrFromType>('from')
  const [filteredAssetList, setFilteredAssetList] = React.useState<AccountAssetOptionType[]>(assetOptions)

  const onChangeSwapView = (view: BuySendSwapViewTypes, option?: ToOrFromType) => {
    if (option) {
      setIsSelectingAsset(option)
      setSwapView(view)
    } else {
      setSwapView(view)
    }
  }

  const onClickSelectNetwork = (network: EthereumChain) => () => {
    onSelectNetwork(network)
    setSwapView('swap')
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setSwapView('swap')
  }

  const onSelectAsset = (asset: AccountAssetOptionType) => () => {
    onSelectSwapAsset(asset, isSelectingAsset)
    setSwapView('swap')
  }

  const onFilterAssetList = (asset: AccountAssetOptionType) => {
    const newList = assetOptions.filter((assets) => assets !== asset)
    setFilteredAssetList(newList)
  }

  const onInputChange = (value: string, name: string) => {
    if (name === 'to') {
      onSetToAmount(value)
    }
    if (name === 'from') {
      onSetFromAmount(value)
    }
    if (name === 'rate') {
      onSetExchangeRate(value)
    }
  }

  const goBack = () => {
    setSwapView('swap')
  }

  return (
    <>
      {swapView === 'swap' &&
        <>
          <Header
            selectedAccount={selectedAccount}
            selectedNetwork={selectedNetwork}
            onChangeSwapView={onChangeSwapView}
          />
          <Swap
            toAsset={swapToAsset}
            fromAsset={swapFromAsset}
            toAmount={toAmount}
            fromAmount={fromAmount}
            orderType={orderType}
            exchangeRate={exchangeRate}
            slippageTolerance={slippageTolerance}
            orderExpiration={orderExpiration}
            onInputChange={onInputChange}
            onFlipAssets={flipSwapAssets}
            onSubmitSwap={onSubmitSwap}
            onSelectPresetAmount={onSelectPresetAmount}
            onSelectSlippageTolerance={onSelectSlippageTolerance}
            onSelectExpiration={onSelectExpiration}
            onChangeSwapView={onChangeSwapView}
            onToggleOrderType={onToggleOrderType}
            onFilterAssetList={onFilterAssetList}
            fromAssetBalance={fromAssetBalance}
            toAssetBalance={toAssetBalance}
          />
        </>
      }
      {swapView !== 'send' &&
        <AccountsAssetsNetworks
          accounts={accounts}
          networkList={networkList}
          goBack={goBack}
          assetOptions={filteredAssetList}
          onClickSelectAccount={onClickSelectAccount}
          onClickSelectNetwork={onClickSelectNetwork}
          onSelectedAsset={onSelectAsset}
          selectedView={swapView}
        />
      }
    </>
  )
}

export default SwapTab
