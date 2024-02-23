// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Row, Column } from '../../../../../components/shared/style'
import {
  Container,
  Title,
  NetworkIcon,
  ArrowIcon,
  NetworkIcons,
  Description
} from './account-type.style'

interface Props {
  title: string
  description: string
  icons: Array<string | React.ReactNode>
  onClick: () => void
}

export const AccountType = ({ title, description, icons }: Props) => {
  return (
    <Container>
      <Row width='100%'>
        <Column
          alignItems='flex-start'
          width='100%'
        >
          <Row
            justifyContent='space-between'
            marginBottom='12px'
          >
            <Row
              justifyContent='flex-start'
              gap='12px'
            >
              <Title>{title}</Title>
              <NetworkIcons>
                {icons.map((icon, index) => (
                  <React.Fragment key={index}>
                    {typeof icon === 'string' ? (
                      <NetworkIcon name={icon} />
                    ) : (
                      { icon }
                    )}
                  </React.Fragment>
                ))}
              </NetworkIcons>
            </Row>
            <ArrowIcon />
          </Row>
          <Description>{description}</Description>
        </Column>
      </Row>
    </Container>
  )
}
