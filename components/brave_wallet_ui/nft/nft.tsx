// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render } from 'react-dom'
import { BrowserRouter } from 'react-router-dom'
import { initLocale } from 'brave-ui'
import { loadTimeData } from '../../common/loadTimeData'

// css
import 'emptykit.css'
import 'chrome://resources/brave/fonts/poppins.css'
import 'chrome://resources/brave/fonts/inter.css'
import './css/nft-global.css'

// theme setup
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import walletDarkTheme from '../theme/wallet-dark'
import walletLightTheme from '../theme/wallet-light'

// utils
import {
  braveWalletOrigin,
  braveWalletPanelOrigin,
  CommandMessage,
  DisplayMode,
  NftUiCommand,
  UpdateLoadingMessage,
  UpdateNFtMetadataErrorMessage,
  UpdateNFtMetadataMessage
} from './nft-ui-messages'
import { NFTMetadataReturnType } from '../constants/types'

// components
import { NftContent } from './components/nft-content/nft-content'

const App = () => {
  const [loadingNftMetadata, setLoadingNftMetadata] =
    React.useState<boolean>(true)
  const [displayMode, setDisplayMode] = React.useState<DisplayMode>()
  const [nftMetadata, setNftMetadata] = React.useState<NFTMetadataReturnType>()
  const [imageUrl, setImageUrl] = React.useState<string>()

  // handle postMessage from wallet ui by setting component state
  // each message has a payload parameter containing the event data
  const onMessageEventListener = React.useCallback(
    (event: MessageEvent<CommandMessage>) => {
      // validate message origin
      if (
        event.origin === braveWalletOrigin ||
        event.origin === braveWalletPanelOrigin
      ) {
        const message = event.data
        switch (message.command) {
          case NftUiCommand.UpdateLoading: {
            const { payload } = message as UpdateLoadingMessage
            setLoadingNftMetadata(payload)
            break
          }

          case NftUiCommand.UpdateNFTMetadata: {
            const { payload } = message as UpdateNFtMetadataMessage
            setDisplayMode(payload.displayMode)

            if (payload.displayMode === 'icon') {
              setImageUrl(payload.icon)
            }

            if (
              payload.displayMode === 'grid' ||
              payload.displayMode === 'details'
            ) {
              setNftMetadata(payload.nftMetadata)
            }

            break
          }

          case NftUiCommand.UpdateNFTMetadataError: {
            const { payload } = message as UpdateNFtMetadataErrorMessage
            setDisplayMode(payload.displayMode)

            break
          }
        }
      }
    },
    []
  )

  React.useEffect(() => {
    // add event listener for postMessage from wallet ui
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
          nftMetadata={nftMetadata}
          imageUrl={imageUrl}
          displayMode={displayMode}
        />
      </BraveCoreThemeProvider>
    </BrowserRouter>
  )
}

function initialize() {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
