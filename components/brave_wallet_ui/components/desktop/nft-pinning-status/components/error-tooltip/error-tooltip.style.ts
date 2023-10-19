// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

export const StyledWrapper = styled.div`
  position: absolute;
  display: flex;
  flex-direction: row;
  align-items: center;
  z-index: 3;
  background-color: ${leo.color.white};
  @media (prefers-color-scheme: dark) {
    background-color: ${leo.color.black};
  }
  width: 280px;
  border-radius: 6px;
  bottom: 30px;
  right: -139px;
  box-shadow: ${leo.effect.elevation['03']};
`

export const TooltipContent = styled.div`
  position: relative;
  padding: 24px;
  width: 100%;
  height: 100%;
  color: ${leo.color.text.primary};
`

export const Heading = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  padding: 0;
  margin: 0;
`

export const List = styled.ul`
  margin: 0%;
  padding-left: 20px;

  li {
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 400;
    font-size: 12px;
    line-height: 20px;
    letter-spacing: 0.01em;
    padding: 0;
    margin: 0;
  }
`

export const ArrowDown = styled.div`
  width: 0;
  height: 0;
  border-left: 8px solid transparent;
  border-right: 8px solid transparent;
  position: absolute;
  border-top: 8px solid ${leo.color.white};
  @media (prefers-color-scheme: dark) {
    border-top: 8px solid ${leo.color.black};
  }
  position: absolute;
  right: 50%;
  bottom: -8px;
`
