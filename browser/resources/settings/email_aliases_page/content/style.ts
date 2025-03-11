// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { color, spacing } from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import styled from 'styled-components'

export const BraveIconCircle = styled(Icon)`
  align-items: center;
  border-radius: 50%;
  border: ${color.divider.subtle} 1px solid;
  display: flex;
  min-height: 4.5em;
  justify-content: center;
  margin-inline-end: 1.5em;
  min-width: 4.5em;
  flex-grow: 0;
  --leo-icon-size: 3.6em;
`

export const Card = styled.div`
  background-color: ${color.container.background};
  border: none;
  overflow: hidden;
  padding: ${spacing.l};
`

export const Row = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  column-gap: 1em;
`

export const Col = styled.div`
  display: flex;
  flex-direction: column;
`
