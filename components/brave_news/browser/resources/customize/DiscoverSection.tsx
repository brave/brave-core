// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import Flex from '$web-common/Flex'

interface Props {
  name?: string
  subtitle?: React.ReactNode
  children?: React.ReactNode
}

const Container = styled(Flex)`
  padding: 16px 0;
`

const Header = styled.span`
  font-weight: 600;
  font-size: 16px;
  margin: 8px 0;
`

const Subtitle = styled.span`
  font-size: 12px;
`

const ItemsContainer = styled.div`
  margin: 8px 0;
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 208px));
  gap: 16px;
`

export default function DiscoverSection (props: Props) {
  return <Container direction='column'>
    {props.name && <Flex direction='row' gap={8} align='center'>
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
