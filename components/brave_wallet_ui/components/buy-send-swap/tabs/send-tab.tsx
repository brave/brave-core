import * as React from 'react'
import {
  UserAccountType,
  AssetOptionType,
  BuySendSwapViewTypes,
  ToOrFromType,
  EthereumChain
} from '../../../constants/types'
import { AssetOptions } from '../../../options/asset-options'
import {
  AccountsAssetsNetworks,
  Header,
  Send
} from '..'

export interface Props {
  accounts: UserAccountType[]
  networkList: EthereumChain[]
  selectedAsset: AssetOptionType
  selectedNetwork: EthereumChain
  selectedAccount: UserAccountType
  selectedAssetAmount: string
  selectedAssetBalance: string
  toAddress: string
  showHeader?: boolean
  onSubmit: () => void
  onSelectNetwork: (network: EthereumChain) => void
  onSelectAccount: (account: UserAccountType) => void
  onSelectAsset: (asset: AssetOptionType, toOrFrom: ToOrFromType) => void
  onSetSendAmount: (value: string) => void
  onSetToAddress: (value: string) => void
  onSelectPresetAmount: (percent: number) => void
}

function SendTab (props: Props) {
  const {
    accounts,
    networkList,
    selectedAsset,
    selectedNetwork,
    selectedAccount,
    selectedAssetAmount,
    selectedAssetBalance,
    toAddress,
    showHeader,
    onSubmit,
    onSelectNetwork,
    onSelectAccount,
    onSelectAsset,
    onSetSendAmount,
    onSetToAddress,
    onSelectPresetAmount
  } = props
  const [sendView, setSendView] = React.useState<BuySendSwapViewTypes>('send')

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    setSendView(view)
  }

  const onClickSelectNetwork = (network: EthereumChain) => () => {
    onSelectNetwork(network)
    setSendView('send')
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setSendView('send')
  }

  const onSelectedAsset = (asset: AssetOptionType) => () => {
    onSelectAsset(asset, 'from')
    setSendView('send')
  }

  const onInputChange = (value: string, name: string) => {
    if (name === 'from') {
      onSetSendAmount(value)
    }
    if (name === 'address') {
      onSetToAddress(value)
    }
  }

  const goBack = () => {
    setSendView('send')
  }

  return (
    <>
      {sendView === 'send' &&
        <>
          {showHeader &&
            <Header
              selectedAccount={selectedAccount}
              selectedNetwork={selectedNetwork}
              onChangeSwapView={onChangeSendView}
            />
          }
          <Send
            selectedAssetAmount={selectedAssetAmount}
            selectedAsset={selectedAsset}
            selectedAssetBalance={selectedAssetBalance}
            toAddress={toAddress}
            onChangeSendView={onChangeSendView}
            onInputChange={onInputChange}
            onSelectPresetAmount={onSelectPresetAmount}
            onSubmit={onSubmit}
          />
        </>
      }
      {sendView !== 'send' &&
        <AccountsAssetsNetworks
          accounts={accounts}
          networkList={networkList}
          goBack={goBack}
          assetOptions={AssetOptions}
          onClickSelectAccount={onClickSelectAccount}
          onClickSelectNetwork={onClickSelectNetwork}
          onSelectedAsset={onSelectedAsset}
          selectedView={sendView}
        />
      }
    </>
  )
}

export default SendTab
