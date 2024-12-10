// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import LeoInput from '@brave/leo/react/input'
import LeoDialog from '@brave/leo/react/dialog'
import {
  layoutPanelWidth //
} from '../../../../../components/desktop/wallet-page-wrapper/wallet-page-wrapper.style'

export const Dialog = styled(LeoDialog).attrs({
  size: window.innerWidth <= layoutPanelWidth ? 'mobile' : 'normal'
})`
  --leo-dialog-padding: 32px;
  --leo-dialog-width: 600px;
  --leo-dialog-backdrop-filter: blur(8px);
`

export const Input = styled(LeoInput)`
  width: 100%;
  max-width: 400px;
`
