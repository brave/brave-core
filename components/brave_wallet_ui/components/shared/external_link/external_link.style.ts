// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Button from '@brave/leo/react/button'

export const LinkButton = styled(Button)<{ width?: string }>`
  --leo-button-radius: 12px;
  width: ${({ width }) => width ?? 'auto'};
`
