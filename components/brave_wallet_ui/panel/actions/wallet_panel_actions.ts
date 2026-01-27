/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createAction } from '@reduxjs/toolkit'
import { BraveWallet } from '../../constants/types'
import { PanelSliceActions } from '../slices/panel.slice'

// Async actions (handled by async handler middleware)
export const visibilityChanged = createAction<boolean>('visibilityChanged')
export const openWalletSettings = createAction('openWalletSettings')
export const navigateToMain = createAction('navigateToMain')
export const cancelConnectHardwareWallet =
  createAction<BraveWallet.AccountInfo>('cancelConnectHardwareWallet')

// Re-export sync actions from slice
export const {
  navigateTo,
  showConnectToSite,
  setHardwareWalletInteractionError,
  setSelectedTransactionId,
} = PanelSliceActions
