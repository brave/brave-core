import * as React from 'react'
import {
  UserAccountType,
  BuySendSwapViewTypes,
  ToOrFromType,
  BraveWallet,
  AmountValidationErrorType
} from '../../../constants/types'
import {
  AccountsAssetsNetworks,
  Header,
  Send
} from '..'

export interface Props {
  accounts: UserAccountType[]
  selectedAsset: BraveWallet.BlockchainToken
  selectedNetwork: BraveWallet.EthereumChain
  selectedAccount: UserAccountType
  selectedAssetAmount: string
  selectedAssetBalance: string
  assetOptions: BraveWallet.BlockchainToken[]
  toAddressOrUrl: string
  toAddress: string
  showHeader?: boolean
  addressError: string
  addressWarning: string
  amountValidationError?: AmountValidationErrorType
  onSubmit: () => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onSelectAccount: (account: UserAccountType) => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken, toOrFrom: ToOrFromType) => void
  onSetSendAmount: (value: string) => void
  onSetToAddressOrUrl: (value: string) => void
  onSelectPresetAmount: (percent: number) => void
  networkList: BraveWallet.EthereumChain[]
  onAddNetwork: () => void
  onAddAsset: () => void
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
    toAddressOrUrl,
    toAddress,
    showHeader,
    assetOptions,
    addressError,
    addressWarning,
    amountValidationError,
    onSubmit,
    onSelectNetwork,
    onSelectAccount,
    onSelectAsset,
    onSetSendAmount,
    onSetToAddressOrUrl,
    onSelectPresetAmount,
    onAddNetwork,
    onAddAsset
  } = props
  const [sendView, setSendView] = React.useState<BuySendSwapViewTypes>('send')

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    setSendView(view)
  }

  const onClickSelectNetwork = (network: BraveWallet.EthereumChain) => () => {
    onSelectNetwork(network)
    setSendView('send')

    // Reset amount to 0
    onSetSendAmount('0')
  }

  const onClickSelectAccount = (account: UserAccountType) => () => {
    onSelectAccount(account)
    setSendView('send')
  }

  const onSelectedAsset = (asset: BraveWallet.BlockchainToken) => () => {
    onSelectAsset(asset, 'from')
    setSendView('send')
  }

  const onInputChange = (value: string, name: string) => {
    if (name === 'from') {
      onSetSendAmount(value)
    }
    if (name === 'address') {
      onSetToAddressOrUrl(value)
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
            toAddressOrUrl={toAddressOrUrl}
            toAddress={toAddress}
            addressError={addressError}
            addressWarning={addressWarning}
            amountValidationError={amountValidationError}
            onChangeSendView={onChangeSendView}
            onInputChange={onInputChange}
            onSelectPresetAmount={onSelectPresetAmount}
            onSubmit={onSubmit}
          />
        </>
      }
      {sendView !== 'send' &&
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
          selectedView={sendView}
          onAddNetwork={onAddNetwork}
          onAddAsset={onAddAsset}
        />
      }
    </>
  )
}

export default SendTab
