// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
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

function SendTab ({
  showHeader
}: Props) {
  // custom hooks
  const { selectSendAsset } = useSend()
  const { sendAssetOptions } = useAssets()

  // redux
  const dispatch = useDispatch()

  // state
  const [sendView, setSendView] = React.useState<BuySendSwapViewTypes>('send')

  // methods
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

  // render
  return (
    <>
      {sendView === 'send' &&
        <>
          {showHeader && <Header onChangeSwapView={setSendView} />}
          <Send onChangeSendView={setSendView} />
        </>
      }
      {sendView !== 'send' &&
        <AccountsAssetsNetworks
          goBack={goBack}
          assetOptions={sendAssetOptions}
          onClickSelectAccount={onClickSelectAccount}
          onSelectedAsset={onSelectedAsset}
          selectedView={sendView}
        />
      }
    </>
  )
}

export default SendTab
