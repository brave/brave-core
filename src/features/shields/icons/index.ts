/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from '../../../theme'

import { ComponentType } from 'react'
import { AlertShieldIcon, OpenNewIcon } from '../../../components/icons'
import palette from '../../../theme/palette'

export const LinkIcon = styled(OpenNewIcon as ComponentType)`
  box-sizing: border-box;
  width: 24px;
  height: 24px;
  padding: 4px;

  * {
    fill: ${palette.blue200};
  }
`

export const ShieldIcon = styled(AlertShieldIcon as ComponentType)`
  box-sizing: border-box;
  width: 48px;
  height: 48px;
  display: block;
  margin: auto;

  * {
    fill: ${palette.grey600};
  }
`
