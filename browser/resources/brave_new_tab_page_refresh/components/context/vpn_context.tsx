/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { VPNModel, VPNModelState, defaultModel } from '../../models/vpn_model'
import { useModelState } from '../../lib/use_model_state'

const Context = React.createContext<VPNModel>(defaultModel())

interface Props {
  model: VPNModel
  children: React.ReactNode
}

export function VPNContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

export function useVPNModel(): VPNModel {
  return React.useContext(Context)
}

export function useVPNState<T>(map: (state: VPNModelState) => T): T {
  return useModelState(useVPNModel(), map)
}
