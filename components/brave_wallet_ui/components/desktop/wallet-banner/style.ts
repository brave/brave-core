// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import LeoAlert from '@brave/leo/react/alert'
import LeoIcon from '@brave/leo/react/icon'

// Shared Styles
import {
  layoutPanelWidth //
} from '../wallet-page-wrapper/wallet-page-wrapper.style'
import { Row } from '../../shared/style'

export const StyledWrapper = styled(Row)`
  overflow: hidden;
  margin-bottom: 16px;
  position: relative;
  padding: 0px 12px;
  @media screen and (max-width: ${layoutPanelWidth}px) {
    padding: 0px 16px;
  }
`

export const Alert = styled(LeoAlert)`
  width: 100%;
  --leo-alert-padding: 16px;
`

export const Icon = styled(LeoIcon)`
  --leo-icon-size: 20px;
`
