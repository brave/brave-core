// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// svgs
import CheckmarkSvg from '../../../assets/svg-icons/big-checkmark.svg'

export const PasswordMatchRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: right;
  width: 100%;
  padding: 0px 12px;
`

export const PasswordMatchText = styled.p`
  color: ${(p) => p.theme.color.interactive05};
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 20px;
  text-align: right;
  letter-spacing: 0.01em;
`

export const PasswordMatchCheckmark = styled.div`
  display: inline-block;
  width: 10px;
  height: 10px;
  margin-right: 4px;
  background-color: ${(p) => p.theme.color.interactive05};
  mask: url(${CheckmarkSvg}) no-repeat 50% 50%;
  mask-size: contain;
  vertical-align: middle;
`

export const TooltipWrapper = styled.div`
  display: flex;
  position: relative;
  flex-direction: row;
  justify-content: flex-end;
  width: calc(100% - 90px);
`
