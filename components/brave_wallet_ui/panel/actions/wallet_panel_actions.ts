/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction } from 'redux-act'
import { InitializedPayloadType, AccountPayloadType } from '../constants/action_types'

export const initialize = createAction('initialize')
export const initialized = createAction<InitializedPayloadType>('initialized')
export const connectToSite = createAction<AccountPayloadType>('connectToSite')
export const cancelConnectToSite = createAction('cancelConnectToSite')
export const visibilityChanged = createAction<boolean>('visibilityChanged')
