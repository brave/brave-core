import * as React from 'react'
import { useDispatch } from 'react-redux'
import {
  BuySendSwapViewTypes,
  BraveWallet,
  WalletAccountType
} from '../../../constants/types'
import {
  AccountsAssetsNetworks,
  Header,
  Send
} from '..'
import { useAssets, useSend } from '../../../common/hooks'
import { WalletActions } from '../../../common/actions'

export interface Props {
  showHeader?: boolean
}

function SendTab (props: Props) {
  const { showHeader } = props

  const {
    selectSendAsset,
    setSendAmount
  } = useSend()

  const { sendAssetOptions } = useAssets()

  const dispatch = useDispatch()

  const [sendView, setSendView] = React.useState<BuySendSwapViewTypes>('send')

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    setSendView(view)
  }

  const onClickSelectNetwork = (network: BraveWallet.NetworkInfo) => () => {
    dispatch(WalletActions.selectNetwork(network))
    setSendView('send')

    // Reset amount to 0
    setSendAmount('0')
  }

  const onClickSelectAccount = (account: WalletAccountType) => () => {
    dispatch(WalletActions.selectAccount(account))
    setSendView('send')
  }

  const onSelectedAsset = (asset: BraveWallet.BlockchainToken) => () => {
    selectSendAsset(asset)
    setSendView('send')
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
              onChangeSwapView={onChangeSendView}
            />
          }
          <Send
            onChangeSendView={onChangeSendView}
          />
        </>
      }
      {sendView !== 'send' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={sendAssetOptions}
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
