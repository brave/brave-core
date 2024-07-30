// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Label from '@brave/leo/react/label'
import Icon from '@brave/leo/react/icon'

// styles
import {
  layoutPanelWidth //
} from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const DappsGrid = styled.div`
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  column-gap: ${leo.spacing['3Xl']};
  height: auto;

  @media screen and (max-width: ${layoutPanelWidth}px) {
    grid-template-columns: 1fr;
  }
`

export const CategoryHeader = styled.p`
  width: 100%;
  font: ${leo.font.heading.h4};
  color: ${leo.color.text.primary};
  text-align: left;
  text-transform: capitalize;
  padding: 0;
  margin: 0;
`

export const PlainButton = styled(Button).attrs({
  kind: 'plain'
})`
  --leo-button-color: ${leo.color.text.interactive};
  --leo-button-padding: 8px;
  white-space: nowrap;
`

export const FilterLabel = styled(Label).attrs({
  color: 'neutral',
  mode: 'default'
})`
  --leo-label-padding: 8px;
  --leo-label-icon-size: 14px;

  color: ${leo.color.neutral[50]};
  font: ${leo.font.xSmall.regular};
  font-weight: 700;
  text-transform: uppercase;
`

export const FilterClose = styled(Icon).attrs({
  name: 'close'
})`
  --leo-icon-size: 14px;
`

export const FilterButton = styled(Button).attrs({
  kind: 'plain-faint',
  size: 'tiny'
})`
  --leo-button-padding: 0;
  --leo-button-radius: 4px;
  flex-grow: 0;
  height: 20px;
`
