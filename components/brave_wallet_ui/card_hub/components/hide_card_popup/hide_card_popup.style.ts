// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import LeoDialog from '@brave/leo/react/dialog'
import styled from 'styled-components'

export const Dialog = styled(LeoDialog)`
  --leo-dialog-padding: ${leo.spacing.xl};
  --leo-dialog-width: 320px;
`
