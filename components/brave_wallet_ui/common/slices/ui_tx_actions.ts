// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { createAction } from '@reduxjs/toolkit'

import {
  SerializableTransactionInfo,
  TransactionInfoLookup,
} from '../../constants/types'

/**
 * Standalone UI actions for tx confirm/status state.
 * Kept outside ui.slice so api endpoints can dispatch them without a
 * ui.slice ↔ api.slice circular import.
 */
export const setSelectedTransactionId = createAction<
  TransactionInfoLookup | undefined
>('ui/setSelectedTransactionId')

export const setSubmittingTransaction = createAction<
  SerializableTransactionInfo | undefined
>('ui/setSubmittingTransaction')
