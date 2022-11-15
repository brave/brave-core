/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const StyledWrapper = styled.div`
  flex: 1;
  display: flex;
  position: relative;
  height: 100%;
`

export const Tip = styled.div`
  position: absolute;
  border-radius: 4px;
  left: 50%;
  transform: translateX(calc(-100% + 15px)) translateY(25%);
  padding: 10px;
  line-height: 18px;
  min-width: 168px;
  color: ${(p) => p.theme.palette.white};
  background: ${(p) => p.theme.palette.black};
  z-index: 120;
  white-space: nowrap;
  font-family: Poppins;
  font-size: 12px;
  letter-spacing: 0.01em;
  top: 9px;
`

export const Pointer = styled.div`
  width: 0;
  height: 0;
  border-style: solid;
  position: absolute;
  left: 50%;
  top: 19px;
  transform: translateX(-50%) translateY(25%);
  border-width: 0 7px 8px 7px;
  z-index: 120;
  border-color: transparent transparent ${(p) => p.theme.palette.black} transparent;
`
