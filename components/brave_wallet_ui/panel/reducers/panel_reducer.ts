/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import {
  BraveWallet,
  CoinType,
  OriginInfo,
  PanelState,
  PanelTypes,
  SignMessageRequest
} from '../../constants/types'
import * as PanelActions from '../actions/wallet_panel_actions'
import {
  ShowConnectToSitePayload
} from '../constants/action_types'
import { PanelTitles } from '../../options/panel-titles'
import { HardwareWalletResponseCodeType } from '../../common/hardware/types'

const defaultOriginInfo: OriginInfo = {
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
      activeRpcEndpointIndex: 0,
      rpcEndpoints: [{ url: 'https://mainnet-infura.brave.com/' }],
      blockExplorerUrls: [],
      iconUrls: [],
      symbol: 'ETH',
      symbolName: 'Ethereum',
      decimals: 18,
      coin: CoinType.ETH,
      supportedKeyrings: [BraveWallet.KeyringId.kDefault],
      isEip1559: true
    }
  },
  signMessageData: [{
    originInfo: defaultOriginInfo,
    id: -1,
    address: '',
    signData:
      {
      ethStandardSignData: {
        message: ''
      },
      ethSignTypedData: undefined,
      ethSiweData: undefined,
      solanaSignData: undefined
    },
    coin: CoinType.ETH,
    chainId: ''
  }],
  signAllTransactionsRequests: [],
  signTransactionRequests: [],
  getEncryptionPublicKeyRequest: {
    requestId: '',
    originInfo: defaultOriginInfo,
    address: ''
  },
  decryptRequest: {
    requestId: '',
    originInfo: defaultOriginInfo,
    address: '',
    unsafeMessage: ''
  },
  switchChainRequest: {
    requestId: '',
    originInfo: defaultOriginInfo,
    chainId: ''
  },
  hardwareWalletCode: undefined,
  selectedTransactionId: undefined
}

export const createPanelReducer = (initialState: PanelState) => {
  const reducer = createReducer<PanelState>({}, initialState)
  reducer.on(PanelActions.navigateTo.type, (state: PanelState, selectedPanel: PanelTypes) => {
    const foundTitle = PanelTitles().find((title) => selectedPanel === title.id)
    const panelTitle = foundTitle ? foundTitle.title : ''
    return {
      ...state,
      selectedPanel,
      lastSelectedPanel: state.selectedPanel,
      panelTitle
    }
  })

  reducer.on(PanelActions.navigateBack.type, (state: PanelState) => {
    const selectedPanel = state.lastSelectedPanel === undefined
      ? state.selectedPanel
      : state.lastSelectedPanel

    const foundTitle = PanelTitles().find((title) => selectedPanel === title.id)
    const panelTitle = foundTitle ? foundTitle.title : ''

    return {
      ...state,
      selectedPanel,
      lastSelectedPanel: state.selectedPanel,
      panelTitle
    }
  })

  reducer.on(PanelActions.showConnectToSite.type, (state: any, payload: ShowConnectToSitePayload) => {
    return {
      ...state,
      connectToSiteOrigin: payload.originInfo,
      connectingAccounts: payload.accounts
    }
  })

  reducer.on(PanelActions.addEthereumChain.type, (state: any, request: BraveWallet.AddChainRequest) => {
    return {
      ...state,
      addChainRequest: request
    }
  })

  reducer.on(PanelActions.switchEthereumChain.type, (state: any, request: BraveWallet.SwitchChainRequest) => {
    return {
      ...state,
      switchChainRequest: request
    }
  })

  reducer.on(PanelActions.getEncryptionPublicKey.type, (state: any, request: BraveWallet.GetEncryptionPublicKeyRequest) => {
    return {
      ...state,
      getEncryptionPublicKeyRequest: request
    }
  })

  reducer.on(PanelActions.decrypt.type, (state: any, request: BraveWallet.DecryptRequest) => {
    return {
      ...state,
      decryptRequest: request
    }
  })

  reducer.on(PanelActions.signMessage.type, (state, payload: SignMessageRequest[]) => {
    return {
      ...state,
      signMessageData: payload
    }
  })

  reducer.on(PanelActions.signTransaction.type, (state: PanelState, payload: BraveWallet.SignTransactionRequest[]): PanelState => {
    return {
      ...state,
      signTransactionRequests: payload
    }
  })

  reducer.on(PanelActions.signAllTransactions.type, (state: PanelState, payload: BraveWallet.SignAllTransactionsRequest[]): PanelState => {
    return {
      ...state,
      signAllTransactionsRequests: payload
    }
  })

  reducer.on(PanelActions.setHardwareWalletInteractionError.type, (state: any, payload?: HardwareWalletResponseCodeType) => {
    return {
      ...state,
      hardwareWalletCode: payload
    }
  })

  reducer.on(
    PanelActions.setSelectedTransactionId.type,
    (state: PanelState, payload: string | undefined): PanelState => {
      return {
        ...state,
        selectedTransactionId: payload
      }
    }
  )
  return reducer
}

const reducer = createPanelReducer(defaultState)
export const panelReducer = reducer
export default reducer
