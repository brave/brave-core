// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import ArrowDownIcon from '../../assets/svg-icons/arrow-down-white-icon.svg'
import ArrowUpIcon from '../../assets/svg-icons/arrow-up-white-icon.svg'

export interface StyleProps {
  isDown: boolean
}

export const StyledWrapper = styled.span<StyleProps>`
  display: flex;
  align-items: center;
  padding: 4px 9px;
  border-radius: 8px;
  background-color: ${p => p.isDown ? '#F75A3A' : '#2AC194'};
  width: 62px;
  height: 24px;
`
export const PriceChange = styled.span`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 11px;
  font-weight: 400;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.background01};
  
  @media (prefers-color-scheme: dark) {
    color: ${p => p.theme.color.text};
  }
`

export const ArrowBase = styled.span`
  width: 12px;
  height: 11px;
  background-repeat: no-repeat;
  background-size: contain;
  margin-right: 2px;
  background-position: center center;
`

export const ArrowUp = styled(ArrowBase)`
  background-image: url(${ArrowUpIcon});
`

export const ArrowDown = styled(ArrowBase)`
  background-image: url(${ArrowDownIcon});
`
