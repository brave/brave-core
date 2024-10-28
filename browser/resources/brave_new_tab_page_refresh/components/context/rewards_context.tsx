/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { RewardsModel, RewardsModelState, defaultModel } from '../../models/rewards_model'
import { useModelState } from '../../lib/use_model_state'

const Context = React.createContext<RewardsModel>(defaultModel())

interface Props {
  model: RewardsModel
  children: React.ReactNode
}

export function RewardsContext(props: Props) {
  return (
    <Context.Provider value={props.model}>
      {props.children}
    </Context.Provider>
  )
}

export function useRewardsModel(): RewardsModel {
  return React.useContext(Context)
}

export function useRewardsState<T>(map: (state: RewardsModelState) => T): T {
  return useModelState(useRewardsModel(), map)
}
