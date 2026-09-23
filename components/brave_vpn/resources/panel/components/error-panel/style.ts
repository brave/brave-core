// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Alert from '@brave/leo/react/alert'
import Button from '@brave/leo/react/button'
import { color, spacing } from '@brave/leo/tokens/css/variables'

export const Box = styled.div`
  width: 100%;
  height: 100%;
  background: ${color.container.background};
  overflow: hidden;
`

export const PanelContent = styled.section`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  align-self: stretch;
`

export const TopContent = styled.div`
  display: flex;
  padding: 0 ${spacing.xl} ${spacing.xl} ${spacing.xl};
  flex-direction: column;
  align-items: center;
  gap: ${spacing.m};
  align-self: stretch;
`

export const StyledAlert = styled(Alert)`
  --leo-alert-padding: ${spacing['2Xl']};
  margin-bottom: ${spacing.m};
  align-self: stretch;
`

export const StyledActionButton = styled(Button)`
  align-self: stretch;
`
