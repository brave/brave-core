// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

export const PasswordStrengthTextWrapper = styled.div`
  padding-left: 12px;
  padding-right: 12px;
  margin-bottom: 20px;
`

export const CriteriaCheckContainer = styled.div`
  width: 100%;
  display: flex;
  flex: 1;
  flex-direction: row;
  align-items: baseline;
  justify-content: flex-start;
  gap: 4px;
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
