import * as React from 'react'
import {
  BuySendSwapViewTypes,
  ToOrFromType,
  BraveWallet,
  WalletAccountType
} from '../../../constants/types'
import {
  AccountsAssetsNetworks,
  Header,
  Swap
} from '..'
import { useSwap } from '../../../common/hooks'
import { useDispatch } from 'react-redux'
import { WalletActions } from '../../../common/actions'

function SwapTab () {
  const swap = useSwap()
  const {
    onSelectTransactAsset,
    swapAssetOptions
  } = swap

  // redux
  const dispatch = useDispatch()

  const [swapView, setSwapView] = React.useState<BuySendSwapViewTypes>('swap')
  const [isSelectingAsset, setIsSelectingAsset] = React.useState<ToOrFromType>('from')
  const [filteredAssetList, setFilteredAssetList] = React.useState<BraveWallet.BlockchainToken[]>(swapAssetOptions)

  const onChangeSwapView = (view: BuySendSwapViewTypes, option?: ToOrFromType) => {
    if (option) {
      setIsSelectingAsset(option)
    }
    setSwapView(view)
  }

  const onClickSelectAccount = (account: WalletAccountType) => () => {
    dispatch(WalletActions.selectAccount(account))
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
            setOrderExpiration={swap.setOrderExpiration}
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
            swapProvider={swap.swapProvider}
          />
        </>
      }
      {swapView !== 'swap' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={filteredAssetList}
          onClickSelectAccount={onClickSelectAccount}
          onSelectedAsset={onSelectAsset}
          selectedView={swapView}
        />
      }
    </>
  )
}

export default SwapTab
