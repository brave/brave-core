// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import LeoToggle from '@brave/leo/react/toggle'
import styled from 'styled-components'

// Shared styles
import { Row } from '$wallet/components/shared/style'

export const Toggle = styled(LeoToggle)`
  --leo-toggle-label-flex-direction: row-reverse;
  flex: 1;
`

export const LabelAndIcon = styled(Row)`
  flex: 1;
`
