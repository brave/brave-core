/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ProgressRing from '@brave/leo/react/progressRing'
import { spacing } from '@brave/leo/tokens/css/variables'
import styled, { css } from 'styled-components'

interface Props {
  // When true, fill a larger area suitable for the dialog Suspense fallback.
  fill?: boolean
}

const LoadingContainer = styled.div<{ fill?: boolean }>`
  --leo-progressring-size: ${spacing['4Xl']};

  display: flex;
  align-items: center;
  justify-content: center;
  padding: ${spacing['4Xl']};
  min-height: 120px;

  ${p => p.fill && css`
    --leo-progressring-size: 50px;
    min-height: 400px;
    height: 100%;
  `}
`

export default function Loading(props: Props) {
  return (
    <LoadingContainer fill={props.fill}>
      <ProgressRing />
    </LoadingContainer>
  )
}
