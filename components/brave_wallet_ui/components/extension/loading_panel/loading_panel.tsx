// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Column } from '../../shared/style'
import { Backdrop, Background, FloatingCard } from '../shared-panel-styles'
import {
  LoadingIconBG,
  LoadingIconFG,
  LoadingIconWrapper
} from './loading_panel.styles'

interface Props {
  message?: string
}

export const LoadingPanel: React.FC<Props> = ({ message }) => {
  return (
    <Background>
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
            {message}
          </Column>
        </FloatingCard>
      </Backdrop>
    </Background>
  )
}
