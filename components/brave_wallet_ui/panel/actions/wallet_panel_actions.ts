/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createAction } from '@reduxjs/toolkit'
import { ShowConnectToSitePayload } from '../constants/action_types'
import {
  BraveWallet,
  HardwareWalletResponseCodeType,
  PanelTypes,
  TransactionInfoLookup
} from '../../constants/types'

export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const showConnectToSite =
  createAction<ShowConnectToSitePayload>('showConnectToSite')
export const openWalletSettings = createAction('openWalletSettings')
export const navigateTo = createAction<PanelTypes>('navigateTo')
export const navigateToMain = createAction('navigateToMain')
export const setHardwareWalletInteractionError = createAction<
  HardwareWalletResponseCodeType | undefined
>('setHardwareWalletInteractionError')
export const cancelConnectHardwareWallet =
  createAction<BraveWallet.AccountInfo>('cancelConnectHardwareWallet')
export const setSelectedTransactionId = createAction<
  TransactionInfoLookup | undefined
>('setSelectedTransactionId')
export const setCloseOnDeactivate = createAction<boolean>(
  'setCloseOnDeactivate'
)
