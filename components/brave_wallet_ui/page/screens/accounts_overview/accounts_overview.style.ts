// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Shared Styles
import { Column } from '../../../components/shared/style'

export const AccountsListWrapper = styled(Column)`
  border-radius: ${leo.radius.l};
  border: 1px solid ${leo.color.divider.subtle};
`
