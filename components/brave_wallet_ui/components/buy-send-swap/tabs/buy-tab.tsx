import * as React from 'react'
import {
  UserAccountType,
  AssetOptionType,
  NetworkOptionsType,
  BuySendSwapViewTypes
} from '../../../constants/types'
import { WyreAssetOptions } from '../../../options/wyre-asset-options'
import {
  AccountsAssetsNetworks,
  Header,
  Buy
} from '..'

export interface Props {
  accounts: UserAccountType[]
  selectedNetwork: NetworkOptionsType
  selectedAccount: UserAccountType
  buyAmount: string
  showHeader?: boolean
  onSubmit: (asset: AssetOptionType) => void
  onSelectNetwork: (network: NetworkOptionsType) => void
  onSelectAccount: (account: UserAccountType) => void
  onSetBuyAmount: (value: string) => void
}

function BuyTab (props: Props) {
  const {
    accounts,
    selectedNetwork,
    selectedAccount,
    buyAmount,
    showHeader,
    onSubmit,
    onSelectNetwork,
    onSelectAccount,
    onSetBuyAmount
  } = props
  const [buyView, setBuyView] = React.useState<BuySendSwapViewTypes>('buy')
  const [selectedAsset, setSelectedAsset] = React.useState<AssetOptionType>(WyreAssetOptions[0])

  const onChangeBuyView = (view: BuySendSwapViewTypes) => {
    setBuyView(view)
  }

  const onClickSelectNetwork = (network: NetworkOptionsType) => () => {
    onSelectNetwork(network)
    setBuyView('buy')
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setBuyView('buy')
  }

  const onSelectedAsset = (asset: AssetOptionType) => () => {
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

  return (
    <>
      {buyView === 'buy' &&
        <>
          {showHeader &&
            <Header
              selectedAccount={selectedAccount}
              selectedNetwork={selectedNetwork}
              onChangeSwapView={onChangeBuyView}
            />
          }
          <Buy
            buyAmount={buyAmount}
            selectedAsset={selectedAsset}
            selectedNetwork={selectedNetwork}
            onChangeBuyView={onChangeBuyView}
            onInputChange={onInputChange}
            onSubmit={onSubmitBuy}
          />
        </>
      }
      {buyView !== 'buy' &&
        <AccountsAssetsNetworks
          accounts={accounts}
          goBack={goBack}
          assetOptions={WyreAssetOptions}
          onClickSelectAccount={onClickSelectAccount}
          onClickSelectNetwork={onClickSelectNetwork}
          onSelectedAsset={onSelectedAsset}
          selectedView={buyView}
        />
      }
    </>
  )
}

export default BuyTab
