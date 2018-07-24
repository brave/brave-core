/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'
import { setTheme } from '../../../helpers'

export const StyledWrapper = styled.div`
  max-width: ${(p: Props) => setTheme(p.theme, 'maxWidth') || '254px'};
  width: 100%;
  margin-bottom: 12px;
` as any

export const StyledTitle = styled.div`
  width: 100%;
  font-family: Poppins;
  line-height: normal;
  font-size: 14px;
  font-weight: 500;
  color: #686978;
  margin-bottom: 6px;
` as any
