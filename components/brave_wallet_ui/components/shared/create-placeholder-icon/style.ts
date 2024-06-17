// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export type AssetIconSizes = 'extra-big' | 'big' | 'medium' | 'small' | 'tiny'

const sizeNameToPixels = (size: AssetIconSizes) => {
  switch (size) {
    case 'extra-big':
      return '96px'
    case 'big':
      return '40px'
    case 'medium':
      return '32px'
    case 'small':
      return '24px'
    case 'tiny':
      return '16px'
    default:
      return '16px'
  }
}

export const IconWrapper = styled.div<{
  isPlaceholder: boolean
  panelBackground?: string
  size: AssetIconSizes
  marginLeft: number
  marginRight: number
}>`
  --size: ${(p) => sizeNameToPixels(p.size)};
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  min-width: var(--size);
  height: ${(p) => (p.isPlaceholder ? 'var(--size)' : 'auto')};
  border-radius: ${(p) => (p.isPlaceholder ? '100%' : '0px')};
  margin-right: ${(p) => `${p.marginRight}px`};
  margin-left: ${(p) => `${p.marginLeft}px`};
  background: ${(p) => (p.panelBackground ? p.panelBackground : 'none')};
`

export const PlaceholderText = styled.span<{
  size: AssetIconSizes
}>`
  font-family: Poppins;
  font-size: ${(p) =>
    p.size === 'big' ? '16px' : p.size === 'tiny' ? '10px' : '12px'};
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`
