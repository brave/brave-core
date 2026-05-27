// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Ring from '@brave/leo/react/progressRing'

import { WalletButton } from '../../../../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  margin-top: 10px;
`

export const Heading = styled.h3`
  font: ${leo.font.default.semibold};
text-align: center;
  color: ${leo.color.text.secondary};
  margin: 0 0 8px 0;
  padding: 0;
`

export const Description = styled.span`
  font: ${leo.font.small.semibold};
font: ${leo.font.small.regular};
background: transparent;
  color: ${leo.color.button.background};
  border: none;
  cursor: pointer;
`

export const LoadingRing = styled(Ring)`
  --leo-progressring-size: 24px;
`

export const RefreshText = styled.div`
  color: ${leo.color.text.secondary};
  text-align: center;
  font: ${leo.font.default.regular};
`
