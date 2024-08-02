// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Collapse from '@brave/leo/react/collapse'

// mixins
import { styledScrollbarMixin } from '../../shared/style'

// shared styles
import { MessageBox } from './style'

export const IconButton = styled(Button)`
  background: none;
  border: none;
`

export const TransactionChangeCollapseContent = styled.div`
  max-height: 126px;
  overflow-y: scroll;
  overflow-x: clip;
  ${styledScrollbarMixin}
`

export const TransactionChangeCollapseTitle = styled.div`
  color: ${leo.color.text.secondary};
  font-family: Poppins;
  font-size: 11px;
  font-style: normal;
  font-weight: 600;
  line-height: 16px;
`

export const TransactionChangeCollapse = styled(Collapse)<{
  hasMultipleCategories?: boolean
}>`
  --leo-collapse-shadow: none;
  --leo-collapse-summary-padding: 8px;
  --leo-collapse-content-padding: 0px 8px 8px 8px;
  font: ${leo.font.small.semibold};
`

export const TransactionChangeCollapseContainer = styled.div<{
  hasMultipleCategories?: boolean
}>`
  --leo-collapse-icon-color-hover: ${(p) =>
    p.hasMultipleCategories ? 'unset' : 'rgba(0, 0, 0, 0)'};
  --leo-collapse-icon-color: ${(p) =>
    p.hasMultipleCategories ? 'unset' : 'rgba(0, 0, 0, 0)'};

  min-width: 100%;
  margin-bottom: 8px;

  & > ${TransactionChangeCollapse} {
    --leo-collapse-radius: ${(p) => (p.hasMultipleCategories ? '0px' : '4px')};
    margin-bottom: -4px;
  }
  & > ${TransactionChangeCollapse}:first-child {
    --leo-collapse-radius: ${(p) =>
      p.hasMultipleCategories ? '4px 4px 0px 0px' : '4px'};
  }
  & > ${TransactionChangeCollapse}:last-child {
    --leo-collapse-radius: ${(p) =>
      p.hasMultipleCategories ? '0px 0px 4px 4px' : '4px'};
    margin-bottom: 4px;
  }
`

export const CollapseHeaderDivider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${leo.color.divider.subtle};
  margin-bottom: 8px;
`

export const Divider = styled.div`
  width: 100%;
  height: 1px;
  background-color: ${leo.color.divider.subtle};
  margin-top: 8px;
  margin-bottom: 8px;
`

export const NetworkNameText = styled.p`
  color: ${leo.color.text.secondary};
  font: ${leo.font.small.regular};
  margin: 0px;
`

export const AccountNameAndAddress = styled.div`
  color: ${leo.color.text.tertiary};
  font: ${leo.font.xSmall.regular};
  margin: 0px;
  word-break: break-all;
`

export const SimulatedTxMessageBox = styled(MessageBox)`
  width: 100%;
`
