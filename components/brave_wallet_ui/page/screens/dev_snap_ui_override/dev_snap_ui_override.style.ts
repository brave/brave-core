// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column } from '../../../components/shared/style'

export const SplitPaneColumn = styled(Column)`
  flex: 1 1 0;
  min-width: 0;
`

export const PreviewPanel = styled(Column)`
  max-width: 100%;
  width: 100%;
  border-radius: 16px;
  border: 1px solid ${leo.color.divider.subtle};
  background: ${leo.color.container.background};
  box-sizing: border-box;
`

export const ButtonEventLog = styled.pre`
  margin: 0;
  font-size: 12px;
  font-family: monospace;
  color: ${leo.color.text.secondary};
  white-space: pre-wrap;
`

export const JsonViewPanel = styled.div`
  display: block;
  width: 100%;
  min-width: 0;

  & > div {
    width: 100%;
    min-width: 0;
  }

  max-height: 420px;
  overflow: auto;
  padding: 12px;
  border-radius: 8px;
  border: 1px solid ${leo.color.divider.subtle};
  background: ${leo.color.container.background};
  box-sizing: border-box;
  font-size: 12px;
  line-height: 1.45;
`
