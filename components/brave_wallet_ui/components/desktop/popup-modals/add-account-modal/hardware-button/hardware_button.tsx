// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Row, Column } from '../../../../shared/style'
import {
  Container,
  Title,
  ArrowIcon,
  Description
} from './hardware_button.style'

interface Props {
  title: string
  description: string
  onClick: () => void
}

export const HardwareButton = ({ title, description, onClick }: Props) => {
  return (
    <Container onClick={onClick}>
      <Row width='100%'>
        <Column
          alignItems='flex-start'
          width='100%'
        >
          <Row
            justifyContent='space-between'
            marginBottom='12px'
          >
            <Title>{title}</Title>
            <ArrowIcon />
          </Row>
          <Description>{description}</Description>
        </Column>
      </Row>
    </Container>
  )
}
