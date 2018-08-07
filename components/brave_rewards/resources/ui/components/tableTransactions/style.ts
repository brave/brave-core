/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { setTheme } from '../../../helpers'

export const StyledTHLast = styled.div`
  text-align: right;
  padding-right: 14px;
` as any

export const StyledProvider = styled.span`
  color: #9e9fab;
` as any

export const StyledType = styled.div`
  color: ${(p: {color: string}) => setTheme(p, 'color') || null};
` as any
