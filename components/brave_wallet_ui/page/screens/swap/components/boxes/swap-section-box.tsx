// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

// Styled Components
import { StyledDiv } from '../shared-swap.styles'

interface BoxStyleProps {
  boxType: 'primary' | 'secondary'
}

interface Props extends BoxStyleProps {
  children?: React.ReactNode
}

export const SwapSectionBox = (props: Props) => {
  const { boxType, children } = props

  return <Wrapper boxType={boxType}>{children}</Wrapper>
}

const Wrapper = styled(StyledDiv) <BoxStyleProps>`
  background-color: ${(p) =>
    p.boxType === 'primary'
      ? 'var(--box-background-primary)'
      : 'var(--box-background-secondary)'};
  box-sizing: border-box;
  border-radius: 16px;
  border: ${(p) =>
    p.boxType === 'secondary'
      ? `1px solid ${p.theme.color.divider01}`
      : 'none'};
  height: ${(p) => (p.boxType === 'primary' ? '122px' : 'unset')};
  min-height: ${(p) => (p.boxType === 'secondary' ? '88px' : '114px')};
  padding: 14px 16px 14px 8px;
  width: 100%;
  position: relative;
  @media screen and (max-width: 570px) {
    min-height: ${(p) => (p.boxType === 'secondary' ? '114px' : '122px')};
  }
`
