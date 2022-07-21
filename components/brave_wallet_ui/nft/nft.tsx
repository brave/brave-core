// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { BrowserRouter } from 'react-router-dom'
import { initLocale } from 'brave-ui'
import { loadTimeData } from '../../common/loadTimeData'

// css
import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import './css/nft-global.css'

// theme setup
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'

// utils
import {
  braveWalletOrigin,
  CommandMessage,
  NftUiCommand,
  UpdateLoadingMessage,
  UpdateNFtMetadataMessage,
  UpdateSelectedAssetMessage,
  UpdateTokenNetworkMessage
} from './nft-ui-messages'
import { BraveWallet, NFTMetadataReturnType } from '../constants/types'

// components
import { NftContent } from './components/nft-content/nft-content'

const App = () => {
  const [loadingNftMetadata, setLoadingNftMetadata] = React.useState<boolean>(true)
  const [selectedAsset, setSelectedAsset] = React.useState<BraveWallet.BlockchainToken>()
  const [nftMetadata, setNftMetadata] = React.useState<NFTMetadataReturnType>()
  const [tokenNetwork, setTokenNetwork] = React.useState<BraveWallet.NetworkInfo>()

  const onMessageEventListener = React.useCallback((event: MessageEvent<CommandMessage>) => {
    // validate message origin
    if (event.origin !== braveWalletOrigin) return

    const message = event.data
    switch (message.command) {
      case NftUiCommand.UpdateLoading: {
        const { payload } = message as UpdateLoadingMessage
        setLoadingNftMetadata(payload)
        break
      }

      case NftUiCommand.UpdateSelectedAsset: {
        const { payload } = message as UpdateSelectedAssetMessage
        setSelectedAsset(payload)
        break
      }

      case NftUiCommand.UpdateNFTMetadata: {
        const { payload } = message as UpdateNFtMetadataMessage
        setNftMetadata(payload)
        break
      }

      case NftUiCommand.UpdateTokenNetwork: {
        const { payload } = message as UpdateTokenNetworkMessage
        setTokenNetwork(payload)
        break
      }
    }
  }, [])

  React.useEffect(() => {
    window.addEventListener('message', onMessageEventListener)
    return () => window.removeEventListener('message', onMessageEventListener)
  }, [])

  return (
    <BrowserRouter>
      <BraveCoreThemeProvider
        dark={walletDarkTheme}
        light={walletLightTheme}
      >
      <NftContent
        isLoading={loadingNftMetadata}
        selectedAsset={selectedAsset}
        nftMetadata={nftMetadata}
        tokenNetwork={tokenNetwork}
      />
    </BraveCoreThemeProvider>
    </BrowserRouter>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
