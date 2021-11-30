/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import {
  PanelState,
  SwapErrorResponse,
  SwapResponse,
  SwitchChainRequest
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
    chainId: '0x1',
    chainName: 'Ethereum Mainnet',
    rpcUrls: ['https://mainnet-infura.brave.com/'],
    blockExplorerUrls: [],
    iconUrls: [],
    symbol: 'ETH',
    symbolName: 'Ethereum',
    decimals: 18,
    isEip1559: true
  },
  swapQuote: undefined,
  swapError: undefined,
  signMessageData: [{
    id: -1,
    address: '',
    message: ''
  }],
  switchChainRequest: {
    origin: {
      url: ''
    },
    chainId: ''
  },
  hardwareWalletCode: undefined
}

const reducer = createReducer<PanelState>({}, defaultState)

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

reducer.on(PanelActions.switchEthereumChain, (state: any, request: SwitchChainRequest) => {
  return {
    ...state,
    switchChainRequest: request
  }
})

reducer.on(PanelActions.setPanelSwapQuote, (state: any, payload: SwapResponse) => {
  return {
    ...state,
    swapQuote: payload
  }
})

reducer.on(PanelActions.setPanelSwapError, (state: any, payload?: SwapErrorResponse) => {
  return {
    ...state,
    swapError: payload
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

export default reducer
