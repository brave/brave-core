// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletPanelState } from '../../constants/types'

type State = Omit<WalletPanelState, 'wallet'>

// safe selectors (primitive return types only)
export const hasInitialized = ({ panel }: State) => panel.hasInitialized
export const panelTitle = ({ panel }: State) => panel.panelTitle
export const selectedPanel = ({ panel }: State) => panel.selectedPanel

// unsafe selectors (will cause re-render if not strictly equal "===") (objects and lists)
export const addChainRequest = ({ panel }: State) => panel.addChainRequest
export const connectToSiteOrigin = ({ panel }: State) => panel.connectToSiteOrigin
export const connectingAccounts = ({ panel }: State) => panel.connectingAccounts
export const decryptRequest = ({ panel }: State) => panel.decryptRequest
export const getEncryptionPublicKeyRequest = ({ panel }: State) => panel.getEncryptionPublicKeyRequest
export const hardwareWalletCode = ({ panel }: State) => panel.hardwareWalletCode
export const selectedTransaction = ({ panel }: State) => panel.selectedTransaction
export const signAllTransactionsRequests = ({ panel }: State) => panel.signAllTransactionsRequests
export const signMessageData = ({ panel }: State) => panel.signMessageData
export const signTransactionRequests = ({ panel }: State) => panel.signTransactionRequests
export const suggestedTokenRequest = ({ panel }: State) => panel.suggestedTokenRequest
export const switchChainRequest = ({ panel }: State) => panel.switchChainRequest
