// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// styles
import { Column } from '../../shared/style'
import { Backdrop, Background, FloatingCard } from '../shared-panel-styles'
import {
  LoadingIconBG,
  LoadingIconFG,
  LoadingIconWrapper,
  LoadingMessage,
} from './loading_panel.styles'

interface Props {
  message?: string
}

export const LoadingPanel: React.FC<Props> = ({ message }) => {
  const isPanel = useSafeUISelector(UISelectors.isPanel)
  const isSidePanel = useSafeUISelector(UISelectors.isSidePanel)
  const isOnlyPanel = isPanel && !isSidePanel

  if (!isOnlyPanel) {
    return (
      <Column
        alignItems='center'
        justifyContent='center'
        fullWidth
        fullHeight
      >
        <ProgressRing mode='indeterminate' />
      </Column>
    )
  }

  return (
    <Background data-testid='loading-panel'>
      <Backdrop>
        <FloatingCard>
          <Column
            alignItems='center'
            justifyContent='center'
            gap={24}
            fullWidth
            fullHeight
          >
            <LoadingIconWrapper>
              <LoadingIconFG />
              <LoadingIconBG />
            </LoadingIconWrapper>
            <LoadingMessage>{message}</LoadingMessage>
          </Column>
        </FloatingCard>
      </Backdrop>
    </Background>
  )
}
