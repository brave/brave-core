// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Flex from '$web-common/Flex'
import { font, spacing } from '@brave/leo/tokens/css/variables'

interface Props {
  name?: string
  subtitle?: React.ReactNode
  children?: React.ReactNode
}

const Container = styled(Flex)`
  padding: ${spacing.xl} 0;
`

const Header = styled.span`
  font: ${font.heading.h4};
  margin: ${spacing.m} 0;
`

const Subtitle = styled.span`
  font: ${font.small.regular};
`

const ItemsContainer = styled.div`
  margin: ${spacing.m} 0;
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 208px));
  gap: ${spacing.xl};
`

export default function DiscoverSection (props: Props) {
  return <Container direction='column'>
    {props.name && <Flex direction='row' gap={spacing.m} align='center'>
      <Header>{props.name}</Header>
    </Flex>}
    {props.subtitle && <Subtitle>
      {props.subtitle}
    </Subtitle>}
    <ItemsContainer>
      {props.children}
    </ItemsContainer>
  </Container>
}
