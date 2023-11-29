// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { EntityId } from '@reduxjs/toolkit'

const ChainSelectionContext = React.createContext<
  readonly [EntityId[], React.Dispatch<React.SetStateAction<EntityId[]>>]
>([[], () => {}])

export const ChainSelectionContextProvider = ChainSelectionContext.Provider

export const useChainSelectionContext = () => {
  const context = React.useContext(ChainSelectionContext)
  if (!context) {
    throw new Error(
      'useChainSelectionContext must be used ' +
        'from a component rendered within a ChainSelectionContextProvider'
    )
  }
  return context
}
