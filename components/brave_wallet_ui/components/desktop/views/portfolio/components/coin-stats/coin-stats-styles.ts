// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: 100%;
`

export const StatWrapper = styled.div`
  display: flex;
  flex-direction: column;
`

export const StatValue = styled.div`
  font: ${leo.font.heading.h2};

  display: flex;
  flex-direction: row;
  align-items: center;
  letter-spacing: 0.01em;
  color: ${leo.color.text.primary};
  margin-bottom: 12px;
  justify-content: center;
`

export const StatLabel = styled.div`
  font: ${leo.font.default.regular};
  display: flex;
  align-items: center;
  text-align: center;
  letter-spacing: 0.01em;
  color: ${leo.color.text.secondary};
  justify-content: center;
`

export const Row = styled.div`
  display: flex;
  flex-direction: row;
  margin: 16px 0 16px 0;
  justify-content: space-between;
`

export const Currency = styled.sup`
  font: ${leo.font.small.regular};
  display: flex;
  letter-spacing: 0.01em;
  color: ${leo.color.text.secondary};
  top: -0.8em;
  margin-right: 2px;
`
