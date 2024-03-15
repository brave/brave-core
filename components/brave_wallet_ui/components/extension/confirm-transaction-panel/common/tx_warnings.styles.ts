// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Collapse from '@brave/leo/react/collapse'

// shared styles
import { styledScrollbarMixin } from '../../../shared/style'

interface WarningProps {
  isCritical?: boolean
}

export const WarningCloseIcon = styled(Icon).attrs({
  name: 'close'
})`
  --leo-icon-size: 20px;
`

export const WarningButton = styled(Button)<
  WarningProps & { isInfo?: boolean }
>`
  --leo-button-color: ${(p) =>
    p.isInfo
      ? leo.color.systemfeedback.infoIcon
      : p.isCritical
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.warningIcon};
`

export const WarningCollapse = styled(Collapse)<WarningProps>`
  --leo-collapse-summary-padding: 12px 16px;
  --leo-collapse-content-padding: ${leo.spacing.m} ${leo.spacing['2Xl']};

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

  @supports (color: color-mix(in srgb, transparent, transparent)) {
    --leo-collapse-icon-color-hover: color-mix(
      in srgb,
      var(--leo-collapse-icon-color),
      var(--leo-collapse-background-color) 50%
    );
  }

  font: ${leo.font.small.semibold};

  color: ${(p) =>
    p.isCritical
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.warningText};

  & > * > li {
    font: ${leo.font.small.regular};
    margin-bottom: 8px;
  }
`

export const AlertTitle = styled.span<WarningProps & { isInfo?: boolean }>`
  margin-left: -16px; /* Remove gap from empty icon in grid */
  text-align: start;
  vertical-align: middle;
  color: ${(p) =>
    p.isInfo
      ? leo.color.systemfeedback.infoText
      : p.isCritical
      ? leo.color.systemfeedback.errorText
      : leo.color.systemfeedback.warningText};
  font: ${leo.font.small.semibold};
`

export const CollapseTitle = styled(AlertTitle)`
  margin-left: 0px;
`

export const WarningsList = styled.ul`
  margin: 0;
  max-height: 230px;
  overflow-y: scroll;
  padding: 8px 12px;

  ${styledScrollbarMixin}

  & > li {
    line-height: 18px;
    margin-left: 2%;
    margin-bottom: 14px;
  }
`

export const DismissButton = styled(Button)`
  flex-grow: 0;
`

export const FullWidth = styled.div`
  min-width: 100%;
`
