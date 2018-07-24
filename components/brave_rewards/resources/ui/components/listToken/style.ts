/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'
import { setTheme } from '../../../helpers'

export const StyledWrapper = styled.div`
  position: relative;
  display: flex;
  border-bottom: ${(p: Props) => setTheme(p.theme, 'borderBottom') || '1px solid #d0d6dc'};
  border-top: ${(p: Props) => setTheme(p.theme, 'borderTop') || 'none'};
  justify-content: space-between;
  align-items: baseline;
  align-content: flex-start;
  flex-wrap: nowrap;
  margin-bottom: 8px;
  font-family: Poppins;
` as any

export const StyledTitle = styled.div`
  font-size: 14px;
  line-height: 2.79;
  color: #4b4c5c;
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 60%;
` as any

export const StyledContentWrapper = styled.div`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 40%;
  text-align: right;
` as any
