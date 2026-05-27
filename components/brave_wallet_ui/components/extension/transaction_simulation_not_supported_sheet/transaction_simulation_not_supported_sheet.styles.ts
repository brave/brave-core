// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// styles
import { Column } from '../../shared/style'

export const alertItemGap = leo.spacing.m

export const TitleText = styled.h4`
  color: ${leo.color.text.primary};
  text-align: center;
  font: ${leo.font.heading.h4};
  padding: 8px 32px 0px 32px;
`

export const AlertTextContainer = styled.div`
  color: ${leo.color.systemfeedback.infoText};
  text-align: center;
  font: ${leo.font.small.regular};
`

export const SeeAvailableNetworksLink = styled.a`
  color: ${leo.color.text.interactive};
  text-align: center;
  font: ${leo.font.small.semibold};
  text-decoration: none;
  cursor: pointer;
`

export const FullWidthChildrenColumn = styled(Column)`
  & * {
    width: 100%;
  }
`
