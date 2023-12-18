// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Button from '@brave/leo/react/button'
import Collapse from '@brave/leo/react/collapse'
import Icon from '@brave/leo/react/icon'

// mixins
import { styledScrollbarMixin } from '../../shared/style'

// shared styles
import { MessageBox } from './style'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  height: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
`

export const IconButton = styled(Button)`
  background: none;
  border: none;
`

export const IconSlot = styled.div`
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
`

export const WarningTriangleFilledIcon = styled(Icon).attrs({
  name: 'warning-triangle-filled'
})`
  --leo-icon-size: 20px;
  --leo-icon-color: ${leo.color.systemfeedback.warningIcon};
`

export const CriticalWarningIcon = styled(Icon).attrs({
  name: 'warning-circle-filled'
})`
  --leo-icon-size: 20px;
  --leo-icon-color: ${leo.color.systemfeedback.errorIcon};
`

export const LaunchIcon = styled(Icon).attrs({ name: 'launch' })`
  --leo-icon-size: 14px;
  --leo-icon-color: ${leo.color.icon.interactive};
  margin-bottom: 1px;
`

export const WarningCollapse = styled(Collapse)<{
  isCritical?: boolean
}>`
  --leo-collapse-summary-padding: 0px 16px;
  --leo-collapse-radius: ${leo.radius.m};
  --leo-collapse-shadow: none;
  --leo-collapse-border-color: none;
  --leo-collapse-background-color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorBackground
      : leo.color.systemfeedback.warningBackground};
  --leo-collapse-icon-color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.warningIcon};
  --leo-collapse-icon-color-hover: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.warningIcon};

  font: var(--leo-font-primary-small-semibold);

  color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.warningText};

  & > * > li {
    font: var(--leo-font-primary-small-regular);
    margin-bottom: 8px;
  }
`

export const WarningTitle = styled.p<{
  isCritical?: boolean
}>`
  color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.warningText};
  font: ${leo.font.browser.small.semibold};
  font-family: 'Inter', 'Poppins';
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
  font: var(--leo-font-primary-small-semibold);
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

export const OriginRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  width: 255px;
`

export const SimulationInfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  width: 100%;
`

export const ContractOriginColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  text-align: left;
  gap: 4px;
`

export const NetworkNameText = styled.p`
  color: var(${leo.color.text.secondary});
  font: ${leo.font.small.regular};
  margin: 0px;
`

export const AccountNameAndAddress = styled.div`
  color: var(${leo.color.text.tertiary});
  font: ${leo.font.xSmall.regular};
  margin: 0px;
  word-break: break-all;
`

export const SimulatedTxMessageBox = styled(MessageBox)`
  width: 100%;
`

export const TabsAndContent = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
  width: 100%;
  padding: 0px 24px;
`

export const TabRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-end;
  justify-content: center;
  width: 100%;
  margin-bottom: 10px;
`
