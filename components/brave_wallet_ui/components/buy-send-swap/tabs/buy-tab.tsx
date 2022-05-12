import * as React from 'react'
import {
  UserAccountType,
  BuySendSwapViewTypes,
  BraveWallet,
  DefaultCurrencies
} from '../../../constants/types'

import {
  AccountsAssetsNetworks,
  Header,
  Buy
} from '..'

import { getUniqueAssets } from '../../../utils/asset-utils'

export interface Props {
  networkList: BraveWallet.NetworkInfo[]
  selectedNetwork: BraveWallet.NetworkInfo
  assetOptions: BraveWallet.BlockchainToken[]
  buyAmount: string
  showHeader?: boolean
  defaultCurrencies: DefaultCurrencies
  onSubmit: (asset: BraveWallet.BlockchainToken) => void
  onSelectAccount: (account: UserAccountType) => void
  onSetBuyAmount: (value: string) => void
  selectedBuyOption: BraveWallet.OnRampProvider
  onSelectBuyOption: (optionId: BraveWallet.OnRampProvider) => void
  wyreAssetOptions: BraveWallet.BlockchainToken[]
  rampAssetOptions: BraveWallet.BlockchainToken[]
}

function BuyTab (props: Props) {
  const {
    networkList,
    selectedNetwork,
    buyAmount,
    showHeader,
    assetOptions,
    defaultCurrencies,
    onSubmit,
    onSelectAccount,
    onSetBuyAmount,
    selectedBuyOption,
    onSelectBuyOption,
    wyreAssetOptions,
    rampAssetOptions
  } = props
  const [buyView, setBuyView] = React.useState<BuySendSwapViewTypes>('buy')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken>(assetOptions[0])

  const onChangeBuyView = (view: BuySendSwapViewTypes) => {
    setBuyView(view)
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setBuyView('buy')
  }

  const onSelectedAsset = (asset: BraveWallet.BlockchainToken) => () => {
    setSelectedAsset(asset)
    setBuyView('buy')
  }

  const onInputChange = (value: string, name: string) => {
    onSetBuyAmount(value)
  }

  const onSubmitBuy = () => {
    onSubmit(selectedAsset)
  }

  const goBack = () => {
    setBuyView('buy')
  }

  React.useEffect(() => {
    if (assetOptions.length > 0) {
      setSelectedAsset(assetOptions[0])
    }
  }, [assetOptions])

  const filteredAssetOptions = React.useMemo(() => {
    return getUniqueAssets(assetOptions)
  }, [assetOptions])

  return (
    <>
      {buyView === 'buy' &&
        <>
          {showHeader &&
            <Header
              onChangeSwapView={onChangeBuyView}
            />
          }
          <Buy
            defaultCurrencies={defaultCurrencies}
            buyAmount={buyAmount}
            selectedAsset={selectedAsset}
            selectedNetwork={selectedNetwork}
            onChangeBuyView={onChangeBuyView}
            onInputChange={onInputChange}
            onSubmit={onSubmitBuy}
            networkList={networkList}
            selectedBuyOption={selectedBuyOption}
            onSelectBuyOption={onSelectBuyOption}
            rampAssetOptions={rampAssetOptions}
            wyreAssetOptions={wyreAssetOptions}
          />
        </>
      }
      {buyView !== 'buy' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={filteredAssetOptions}
          onClickSelectAccount={onClickSelectAccount}
          onSelectedAsset={onSelectedAsset}
          selectedView={buyView}
        />
      }
    </>
  )
}

export default BuyTab
