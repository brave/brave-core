// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const StyledWrapper = styled.span`
  flex: 1;
  display: flex;
  position: relative;
  height: 100%;
`

export const Tip = styled.div`
  position: absolute;
  border-radius: 4px;
  left: -65px;
  transform: translateX(calc(-50% - 30px)) translateY(25%);
  padding: 6px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};
  z-index: 120;
  font-family: Poppins;
  font-size: 12px;
  line-height: 16px;
  letter-spacing: 0.01em;
  top: -144px;
  height: 108px;
  width: 240px;
`

export const Pointer = styled.div`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  left: 50%;
  top: -12px;
  transform: translateX(-50%) translateY(25%) rotate(180deg);
  border-width: 0 7px 8px 7px;
  z-index: 120;
  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
`
