// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

interface StyleProps {
  isPlaceholder: boolean
  panelBackground?: string
  size: 'big' | 'medium' | 'small'
  marginLeft: number
  marginRight: number
}

export const IconWrapper = styled.div<StyleProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: ${(p) => p.size === 'big' ? '40px' : p.size === 'medium' ? '32px' : '24px'};
  height: ${(p) => p.isPlaceholder ? p.size === 'big' ? '40px' : '24px' : 'auto'};
  border-radius: ${(p) => p.isPlaceholder ? '100%' : '0px'};
  margin-right: ${(p) => `${p.marginRight}px`};
  margin-left: ${(p) => `${p.marginLeft}px`};
  background: ${(p) => p.panelBackground ? p.panelBackground : 'none'};
`

export const PlaceholderText = styled.span<Partial<StyleProps>>`
  font-family: Poppins;
  font-size: ${(p) => p.size === 'big' ? '16px' : '12px'};
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`
