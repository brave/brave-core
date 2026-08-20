// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { useHistory } from 'react-router-dom'

// Selectors
import { useSafeUISelector } from './use-safe-selector'
import { UISelectors } from '../selectors'

// Utils
import { openWalletRouteTab } from '../../utils/routes-utils'

export const useRoute = () => {
  // Routing
  const history = useHistory()

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)

  // Methods
  const openOrPushRoute = React.useCallback(
    (route: string) => {
      if (isPanel && !isSidePanel) {
        openWalletRouteTab(route)
        return
      }
      history.push(route)
    },
    [history, isPanel, isSidePanel],
  )
  return { openOrPushRoute }
}
