/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import {
  BraveWallet,
  PanelState
} from '../../constants/types'
import * as PanelActions from '../actions/wallet_panel_actions'
import {
  ShowConnectToSitePayload,
  EthereumChainPayload,
  SignMessagePayload
} from '../constants/action_types'
import { PanelTitles } from '../../options/panel-titles'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'

const defaultState: PanelState = {
  hasInitialized: false,
  connectToSiteOrigin: '',
  selectedPanel: 'main',
  panelTitle: '',
  connectingAccounts: [],
  networkPayload: {
    chainId: BraveWallet.MAINNET_CHAIN_ID,
    chainName: 'Ethereum Mainnet',
    rpcUrls: ['https://mainnet-infura.brave.com/'],
    blockExplorerUrls: [],
    iconUrls: [],
    symbol: 'ETH',
    symbolName: 'Ethereum',
    decimals: 18,
    coin: BraveWallet.CoinType.ETH,
    data: {
      ethData: {
        isEip1559: true
      }
    }
  },
  signMessageData: [{
    id: -1,
    address: '',
    message: '',
    isEip712: false,
    domainHash: '',
    primaryHash: ''
  }],
  getEncryptionPublicKeyRequest: {
    address: '',
    message: '',
    origin: {
      url: ''
    }
  },
  switchChainRequest: {
    origin: {
      url: ''
    },
    chainId: ''
  },
  hardwareWalletCode: undefined,
  suggestedToken: undefined
}

export const createPanelReducer = (initialState: PanelState) => {
  const reducer = createReducer<PanelState>({}, initialState)
  reducer.on(PanelActions.navigateTo, (state: any, selectedPanel: string) => {
    const foundTitle = PanelTitles().find((title) => selectedPanel === title.id)
    const panelTitle = foundTitle ? foundTitle.title : ''

    return {
      ...state,
      selectedPanel,
      panelTitle
    }
  })

  reducer.on(PanelActions.showConnectToSite, (state: any, payload: ShowConnectToSitePayload) => {
    return {
      ...state,
      connectToSiteOrigin: payload.origin,
      connectingAccounts: payload.accounts
    }
  })

  reducer.on(PanelActions.addEthereumChain, (state: any, networkPayload: EthereumChainPayload) => {
    return {
      ...state,
      networkPayload: networkPayload.chain
    }
  })

  reducer.on(PanelActions.switchEthereumChain, (state: any, request: BraveWallet.SwitchChainRequest) => {
    return {
      ...state,
      switchChainRequest: request
    }
  })

  reducer.on(PanelActions.getEncryptionPublicKey, (state: any, request: BraveWallet.GetEncryptionPublicKeyRequest) => {
    return {
      ...state,
      getEncryptionPublicKeyRequest: request
    }
  })

  reducer.on(PanelActions.signMessage, (state: any, payload: SignMessagePayload[]) => {
    return {
      ...state,
      signMessageData: payload
    }
  })

  reducer.on(PanelActions.signMessage, (state: any, payload: SignMessagePayload[]) => {
    return {
      ...state,
      signMessageData: payload
    }
  })

  reducer.on(PanelActions.setHardwareWalletInteractionError, (state: any, payload?: HardwareWalletResponseCodeType) => {
    return {
      ...state,
      hardwareWalletCode: payload
    }
  })

  reducer.on(PanelActions.addSuggestToken, (state: any, payload: BraveWallet.AddSuggestTokenRequest) => {
    return {
      ...state,
      suggestedToken: payload.token
    }
  })
  return reducer
}

const reducer = createPanelReducer(defaultState)
export const panelReducer = reducer
export default reducer
