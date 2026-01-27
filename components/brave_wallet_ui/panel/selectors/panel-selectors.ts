// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { createSelector } from '@reduxjs/toolkit'

import { WalletPanelState } from '../../constants/types'

type State = Omit<WalletPanelState, 'wallet'>

const selectPanelState = (state: State) => state.panel

// safe selectors (primitive return types only)
export const hasInitialized = ({ panel }: State) => panel.hasInitialized
export const selectedPanel = ({ panel }: State) => panel.selectedPanel

// memoized selectors (safe for objects and arrays)
export const connectToSiteOrigin = createSelector(
  [selectPanelState],
  (panel) => panel.connectToSiteOrigin,
)
export const connectingAccounts = createSelector(
  [selectPanelState],
  (panel) => panel.connectingAccounts,
)
export const hardwareWalletCode = createSelector(
  [selectPanelState],
  (panel) => panel.hardwareWalletCode,
)
export const selectedTransactionId = createSelector(
  [selectPanelState],
  (panel) => panel.selectedTransactionId,
)
