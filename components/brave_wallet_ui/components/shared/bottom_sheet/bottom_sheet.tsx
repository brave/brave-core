// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Styled Components
import {
  BottomCardWrapper,
  BottomCard,
  CloseButton,
  CloseIcon
} from './bottom_sheet.style'
import { Row } from '../style'

interface Props {
  onClose: () => void
  children?: React.ReactNode
}

export const BottomSheet = (props: Props) => {
  const { onClose, children } = props

  return (
    <BottomCardWrapper>
      <BottomCard fullWidth={true}>
        <Row
          padding='16px 16px 0px 16px'
          justifyContent='flex-end'
        >
          <CloseButton onClick={onClose}>
            <CloseIcon />
          </CloseButton>
        </Row>
        {children}
      </BottomCard>
    </BottomCardWrapper>
  )
}
