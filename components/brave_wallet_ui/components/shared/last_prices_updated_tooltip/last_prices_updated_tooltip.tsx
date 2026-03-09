// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Tooltip from '@brave/leo/react/tooltip'

// Constants
import {
  LOCAL_STORAGE_KEYS, //
} from '../../../common/constants/local-storage-keys'

// Utils
import { formatDateAsRelative } from '../../../utils/datetime-utils'
import { getLocale } from '../../../../common/locale'

interface Props {
  children: React.ReactNode
}

export const LastPricesUpdatedTooltip = ({ children }: Props) => {
  // State
  const [label, setLabel] = React.useState('')

  // Methods
  const onMouseEnter = React.useCallback(() => {
    const lastUpdated = window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.TOKEN_SPOT_PRICES_LAST_UPDATED,
    )
    if (lastUpdated) {
      setLabel(
        getLocale('braveWalletLastUpdatedAgo').replace(
          '$1',
          formatDateAsRelative(new Date(lastUpdated)),
        ),
      )
    }
  }, [])

  return (
    <Tooltip
      text={label}
      hidden={!label}
    >
      <span onMouseEnter={onMouseEnter}>{children}</span>
    </Tooltip>
  )
}
