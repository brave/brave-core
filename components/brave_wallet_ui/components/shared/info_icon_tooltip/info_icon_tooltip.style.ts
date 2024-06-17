// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'

export const InfoIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 16px;
`

export const TooltipTextContent = styled.div<{
  maxWidth?: string
}>`
  max-width: ${(p) => p.maxWidth ?? '248px'};
`
