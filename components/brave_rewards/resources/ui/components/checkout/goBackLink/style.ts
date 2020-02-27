/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

import { CaratStrongLeftIcon } from 'brave-ui/components/icons'

export const Container = styled.span`
  display: inline-block;
  padding-left: 13px;
`

export const LeftIcon = styled(CaratStrongLeftIcon)`
  height: 15px;
  width: 15px;
  display: inline-block;
  vertical-align: middle;
  margin-right: 6px;
`
