import * as React from 'react'
import {
  UserAccountType,
  AssetOptionType,
  NetworkOptionsType,
  BuySendSwapViewTypes,
  ToOrFromType
} from '../../../constants/types'
import { AssetOptions } from '../../../options/asset-options'
import { NetworkOptions } from '../../../options/network-options'
import {
  Header,
  SelectAccount,
  SelectAsset,
  SelectNetwork,
  Send
} from '..'

export interface Props {
  accounts: UserAccountType[]
  selectedAsset: AssetOptionType
  selectedNetwork: NetworkOptionsType
  selectedAccount: UserAccountType
  selectedAssetAmount: string
  selectedAssetBalance: string
  toAddress: string
  showHeader?: boolean
  onSubmit: () => void
  onSelectNetwork: (network: NetworkOptionsType) => void
  onSelectAccount: (account: UserAccountType) => void
  onSelectAsset: (asset: AssetOptionType, toOrFrom: ToOrFromType) => void
  onSetSendAmount: (value: string) => void
  onSetToAddress: (value: string) => void
  onSelectPresetAmount: (percent: number) => void
}

function SendTab (props: Props) {
  const {
    accounts,
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

  const onClickSelectNetwork = (network: NetworkOptionsType) => () => {
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
      {sendView === 'acounts' &&
        <SelectAccount
          accounts={accounts}
          onSelectAccount={onClickSelectAccount}
          onBack={goBack}
        />
      }
      {sendView === 'assets' &&
        <SelectAsset
          assets={AssetOptions}
          onSelectAsset={onSelectedAsset}
          onBack={goBack}
        />
      }
      {sendView === 'networks' &&
        <SelectNetwork
          networks={NetworkOptions}
          onSelectNetwork={onClickSelectNetwork}
          onBack={goBack}
        />
      }
    </>
  )
}

export default SendTab
