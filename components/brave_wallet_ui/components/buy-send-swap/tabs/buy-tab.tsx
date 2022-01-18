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

export interface Props {
  accounts: UserAccountType[]
  networkList: BraveWallet.EthereumChain[]
  selectedNetwork: BraveWallet.EthereumChain
  selectedAccount: UserAccountType
  assetOptions: BraveWallet.BlockchainToken[]
  buyAmount: string
  showHeader?: boolean
  defaultCurrencies: DefaultCurrencies
  onSubmit: (asset: BraveWallet.BlockchainToken) => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onSelectAccount: (account: UserAccountType) => void
  onSetBuyAmount: (value: string) => void
  onAddNetwork: () => void
  onAddAsset: () => void
}

function BuyTab (props: Props) {
  const {
    accounts,
    networkList,
    selectedNetwork,
    selectedAccount,
    buyAmount,
    showHeader,
    assetOptions,
    defaultCurrencies,
    onAddAsset,
    onSubmit,
    onSelectNetwork,
    onSelectAccount,
    onSetBuyAmount,
    onAddNetwork
  } = props
  const [buyView, setBuyView] = React.useState<BuySendSwapViewTypes>('buy')
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken>(assetOptions[0])

  const onChangeBuyView = (view: BuySendSwapViewTypes) => {
    setBuyView(view)
  }

  const onClickSelectNetwork = (network: BraveWallet.EthereumChain) => () => {
    onSelectNetwork(network)
    setBuyView('buy')
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
            defaultCurrencies={defaultCurrencies}
            buyAmount={buyAmount}
            selectedAsset={selectedAsset}
            selectedNetwork={selectedNetwork}
            onChangeBuyView={onChangeBuyView}
            onInputChange={onInputChange}
            onSubmit={onSubmitBuy}
            networkList={networkList}
          />
        </>
      }
      {buyView !== 'buy' &&
        <AccountsAssetsNetworks
          selectedAccount={selectedAccount}
          selectedNetwork={selectedNetwork}
          accounts={accounts}
          networkList={networkList}
          goBack={goBack}
          assetOptions={assetOptions}
          onClickSelectAccount={onClickSelectAccount}
          onClickSelectNetwork={onClickSelectNetwork}
          onSelectedAsset={onSelectedAsset}
          selectedView={buyView}
          onAddNetwork={onAddNetwork}
          onAddAsset={onAddAsset}
        />
      }
    </>
  )
}

export default BuyTab
