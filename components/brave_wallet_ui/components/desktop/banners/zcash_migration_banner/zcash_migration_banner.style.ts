// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import LeoAlert from '@brave/leo/react/alert'
import LeoButton from '@brave/leo/react/button'

// Shared Styles
import { Row } from '../../../shared/style'
import {
  layoutPanelWidth, //
} from '../../wallet-page-wrapper/wallet-page-wrapper.style'

export const Wrapper = styled(Row)`
  padding: 0px 32px;
  margin: 0px 0px 32px 0px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 16px 16px 0px 16px;
    margin: 0px;
  }
`

export const Alert = styled(LeoAlert)`
  width: 100%;
  --leo-alert-padding: 16px;
`

export const Button = styled(LeoButton)`
  flex-grow: 0;
`
