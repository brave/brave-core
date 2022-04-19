// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import CheckmarkSvg from '../../../assets/svg-icons/big-checkmark.svg'

export const GreenCheckmark = styled.div`
  display: inline-block;
  width: 10px;
  height: 10px;
  margin-right: 4px;
  background-color: ${(p) => p.theme.color.successBorder};
  mask: url(${CheckmarkSvg}) no-repeat 50% 50%;
  mask-size: contain;
  vertical-align: middle;
`

export const PasswordStrengthTextWrapper = styled.div`
  padding-left: 12px;
  padding-right: 12px;
  margin-bottom: 20px;
`

export const PasswordStrengthText = styled.p<{ isStrong?: boolean }>`
  font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 14px;
  letter-spacing: 0.01em;
  margin-bottom: 6px;
  vertical-align: middle;
  color: ${(p) => p.isStrong ? p.theme.color.successBorder : p.theme.palette.white};
`

export const PasswordStrengthHeading = styled(PasswordStrengthText)`
  font-size: 14px;
`
