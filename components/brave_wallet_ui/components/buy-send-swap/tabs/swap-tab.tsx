import * as React from 'react'
import {
  UserAccountType,
  BuySendSwapViewTypes,
  ToOrFromType,
  BraveWallet
} from '../../../constants/types'
import {
  AccountsAssetsNetworks,
  Header,
  Swap
} from '..'
import { useSwap } from '../../../common/hooks'

export interface Props {
  onSelectNetwork: (network: BraveWallet.NetworkInfo) => void
  onSelectAccount: (account: UserAccountType) => void
  onAddNetwork: () => void
  onAddAsset: () => void
}

function SwapTab (props: Props) {
  const {
    onSelectNetwork,
    onSelectAccount,
    onAddNetwork,
    onAddAsset
  } = props

  const swap = useSwap()
  const {
    onSelectTransactAsset,
    swapAssetOptions
  } = swap

  const [swapView, setSwapView] = React.useState<BuySendSwapViewTypes>('swap')
  const [isSelectingAsset, setIsSelectingAsset] = React.useState<ToOrFromType>('from')
  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>(swapAssetOptions)

  const onChangeSwapView = (view: BuySendSwapViewTypes, option?: ToOrFromType) => {
    if (option) {
      setIsSelectingAsset(option)
    }
    setSwapView(view)
  }

  const onClickSelectNetwork = (network: BraveWallet.NetworkInfo) => () => {
    onSelectNetwork(network)
    setSwapView('swap')
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setSwapView('swap')
  }

  const onSelectAsset = (asset: BraveWallet.BlockchainToken) => () => {
    onSelectTransactAsset(asset, isSelectingAsset)
    setSwapView('swap')
  }

  const onFilterAssetList = React.useCallback((asset?: BraveWallet.BlockchainToken) => {
    if (!asset) {
      return
    }

    const newList = swapAssetOptions.filter((assets) => assets !== asset)
    setFilteredAssetList(newList)
  }, [swapAssetOptions])

  const goBack = () => {
    setSwapView('swap')
  }

  return (
    <>
      {swapView === 'swap' &&
        <>
          <Header
            onChangeSwapView={onChangeSwapView}
          />
          <Swap
            customSlippageTolerance={swap.customSlippageTolerance}
            exchangeRate={swap.exchangeRate}
            flipSwapAssets={swap.flipSwapAssets}
            fromAmount={swap.fromAmount}
            fromAssetBalance={swap.fromAssetBalance}
            isFetchingSwapQuote={swap.isFetchingSwapQuote}
            isSwapButtonDisabled={swap.isSwapButtonDisabled}
            onChangeSwapView={onChangeSwapView}
            onCustomSlippageToleranceChange={swap.onCustomSlippageToleranceChange}
            onFilterAssetList={onFilterAssetList}
            onSelectExpiration={swap.onSelectExpiration}
            onSelectPresetAmount={swap.onSelectPresetAmount}
            onSelectSlippageTolerance={swap.onSelectSlippageTolerance}
            onSubmitSwap={swap.onSubmitSwap}
            onSwapInputChange={swap.onSwapInputChange}
            onSwapQuoteRefresh={swap.onSwapQuoteRefresh}
            onToggleOrderType={swap.onToggleOrderType}
            orderExpiration={swap.orderExpiration}
            orderType={swap.orderType}
            selectedPreset={swap.selectedPreset}
            setSelectedPreset={swap.setSelectedPreset}
            slippageTolerance={swap.slippageTolerance}
            swapValidationError={swap.swapValidationError}
            toAmount={swap.toAmount}
            toAssetBalance={swap.toAssetBalance}
            fromAsset={swap.fromAsset}
            toAsset={swap.toAsset}
          />
        </>
      }
      {swapView !== 'swap' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={filteredAssetList}
          onClickSelectAccount={onClickSelectAccount}
          onClickSelectNetwork={onClickSelectNetwork}
          onSelectedAsset={onSelectAsset}
          selectedView={swapView}
          onAddNetwork={onAddNetwork}
          onAddAsset={onAddAsset}
        />
      }
    </>
  )
}

export default SwapTab
