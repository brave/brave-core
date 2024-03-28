// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Alert from '@brave/leo/react/alert'
import * as leo from '@brave/leo/tokens/css'

export const Wrapper = styled.span`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
`

export const Rectangle = styled.span<{
  isActive: boolean
  width: string
}>`
  width: ${(p) => p.width};
  height: 8px;
  border-radius: 4px;
  background-color: ${(p) =>
    p.isActive ? leo.color.button.background : leo.color.purple[20]};
`

export const InfoAlert = styled(Alert).attrs({
  kind: 'info',
  mode: 'simple'
})`
  --leo-alert-center-position: 'center';
  --leo-alert-center-width: '100%';
  width: 100%;

  leo-alert {
    align-items: center;
  }
`
