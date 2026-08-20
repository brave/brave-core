// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

// Styles
import { Wrapper, Popup } from './confirmation_popup.style'
import { Column } from '../../shared/style'

interface Props {
  children?: React.ReactNode
  isLoading?: boolean
}

export function ConfirmationPopup(props: Props) {
  const { children, isLoading } = props

  return (
    <Wrapper>
      <Popup>
        {isLoading ? (
          <Column
            alignItems='center'
            justifyContent='center'
            fullWidth
            fullHeight
          >
            <ProgressRing mode='indeterminate' />
          </Column>
        ) : (
          children
        )}
      </Popup>
    </Wrapper>
  )
}
