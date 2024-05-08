// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

// Assets
import {
  NoTokensIconLight,
  NoTokensIconDark
} from '../../../../../../assets/svg-icons/empty-state-icons'

// Shared Styles
import { Column, Text } from '../../../../../shared/style'

export const StyledWrapper = styled(Column)`
  max-width: 620px;
`

export const Title = styled(Text)`
  color: ${leo.color.text.primary};
  line-height: 24px;
  margin-bottom: 4px;
`

export const Description = styled(Text)`
  color: ${leo.color.text.secondary};
  line-height: 24px;
  margin-bottom: 4px;
`

export const ButtonWrapper = styled.div`
  display: flex;
  width: 124px;
`

export const EmptyStateIcon = styled.div`
  width: 120px;
  height: 120px;
  background-repeat: no-repeat;
  background-size: 100%;
  background-position: center;
  margin-bottom: 16px;
  background-image: url(${NoTokensIconLight});
  @media (prefers-color-scheme: dark) {
    background-image: url(${NoTokensIconDark});
  }
`
