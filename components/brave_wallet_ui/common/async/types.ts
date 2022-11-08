// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { AnyAction, MiddlewareAPI } from 'redux'
import { ThunkDispatch } from 'redux-thunk'

import { WalletPageState, WalletPanelState } from '../../constants/types'

export type State = WalletPanelState | WalletPageState
export type Dispatch = ThunkDispatch<State, void, AnyAction>
export type Store = MiddlewareAPI<Dispatch, State>
