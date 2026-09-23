// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column } from '../../shared/style'

export const StyledWrapper = styled(Column)`
  background-color: ${leo.color.page.background};
`

export const ArrowIconContainer = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 32px;
  min-height: 32px;
  border-radius: 100%;
  background-color: ${leo.color.container.highlight};
  --leo-icon-size: 16px;
  --leo-icon-color: ${leo.color.icon.default};
`

export const Card = styled(Column)`
  background-color: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
  box-shadow: ${leo.effect.elevation['01']};
  overflow: hidden;
`

export const InfoBox = styled(Column)`
  background-color: ${leo.color.container.highlight};
  border-radius: ${leo.radius.xl};
  overflow: hidden;
`

export const ProviderIcon = styled.img`
  height: 12px;
  width: auto;
`
