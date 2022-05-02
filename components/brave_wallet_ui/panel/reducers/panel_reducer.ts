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
  SignMessagePayload
} from '../constants/action_types'
import { PanelTitles } from '../../options/panel-titles'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'

const defaultOriginInfo: BraveWallet.OriginInfo = {
  origin: {
    scheme: '',
    host: '',
    port: 0,
    nonceIfOpaque: undefined
  },
  originSpec: '',
  eTldPlusOne: ''
}

const defaultState: PanelState = {
  hasInitialized: false,
  connectToSiteOrigin: defaultOriginInfo,
  selectedPanel: 'main',
  panelTitle: '',
  connectingAccounts: [],
  addChainRequest: {
    originInfo: defaultOriginInfo,
    networkInfo: {
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
    }
  },
  signMessageData: [{
    originInfo: defaultOriginInfo,
    id: -1,
    address: '',
    message: '',
    isEip712: false,
    domainHash: '',
    primaryHash: ''
  }],
  getEncryptionPublicKeyRequest: {
    originInfo: defaultOriginInfo,
    address: ''
  },
  decryptRequest: {
    originInfo: defaultOriginInfo,
    address: '',
    unsafeMessage: ''
  },
  switchChainRequest: {
    originInfo: defaultOriginInfo,
    chainId: ''
  },
  hardwareWalletCode: undefined,
  suggestedTokenRequest: undefined,
  selectedTransaction: undefined
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
      connectToSiteOrigin: payload.originInfo,
      connectingAccounts: payload.accounts
    }
  })

  reducer.on(PanelActions.addEthereumChain, (state: any, request: BraveWallet.AddChainRequest) => {
    return {
      ...state,
      addChainRequest: request
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

  reducer.on(PanelActions.decrypt, (state: any, request: BraveWallet.DecryptRequest) => {
    return {
      ...state,
      decryptRequest: request
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

  reducer.on(PanelActions.addSuggestToken, (state: PanelState, payload: BraveWallet.AddSuggestTokenRequest): PanelState => {
    return {
      ...state,
      suggestedTokenRequest: payload
    }
  })
  reducer.on(PanelActions.setSelectedTransaction, (state: PanelState, payload: BraveWallet.TransactionInfo | undefined): PanelState => {
    return {
      ...state,
      selectedTransaction: payload
    }
  })
  return reducer
}

const reducer = createPanelReducer(defaultState)
export const panelReducer = reducer
export default reducer
