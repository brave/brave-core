// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import Icon from '@brave/leo/react/icon'
import Collapse from '@brave/leo/react/collapse'

// shared styles
import { styledScrollbarMixin } from '../../../shared/style'

interface WarningProps {
  isCritical?: boolean
}

export const WarningCloseIcon = styled(Icon).attrs<WarningProps>({
  name: 'close'
})<WarningProps>`
  --leo-icon-size: 20px;
  color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.warningIcon};
`

export const WarningCollapse = styled(Collapse)<WarningProps>`
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

export const WarningTitle = styled.p<WarningProps & { isBold?: boolean }>`
  color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.warningText};
  font: ${(p) =>
    p.isBold
      ? leo.font.browser.small.semibold
      : leo.font.browser.small.regular};
  font-family: 'Inter', 'Poppins';
`

export const WarningsList = styled.ul`
  margin: 0;
  max-height: 230px;
  overflow-y: scroll;
  padding: 8px 12px;

  ${styledScrollbarMixin}

  & > li {
    line-height: 18px;
    margin-bottom: 14px;
  }
`
