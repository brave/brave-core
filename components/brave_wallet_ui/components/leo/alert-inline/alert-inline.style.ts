// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { AlertType } from '../../../constants/types'

import { LeoColors } from './leo-colors'

export const InlineAlertContainer = styled.div<{
  alertType: AlertType
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 16px;

  margin-top: 16px;
  margin-bottom: 16px;
  padding: 16px;
  width: 100%;
  min-height: 56px;

  border-radius: 8px;

  color: ${LeoColors['light.text.primary']};
  background-color: ${({ alertType }) =>
    alertType === 'danger'
      ? LeoColors['light.system.feedback.error.background']
      : alertType === 'warning'
      ? LeoColors['light.system.feedback.warning.background']
      : 'unset'};
  @media (prefers-color-scheme: dark) {
    color: ${LeoColors['dark.text.primary']};
    background-color: ${({ alertType }) =>
      alertType === 'danger'
        ? LeoColors['dark.system.feedback.error.background']
        : alertType === 'warning'
        ? LeoColors['dark.system.feedback.warning.background']
        : 'unset'};
  }
`
