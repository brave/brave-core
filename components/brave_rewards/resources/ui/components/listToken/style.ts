/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'

export const StyledWrapper = styled('div')<Partial<Props>>`
  position: relative;
  display: flex;
  border-bottom: ${p => p.border === 'last' ? 'none' : '1px solid #d0d6dc'};
  border-top: ${p => p.border === 'first' ? '1px solid #d0d6dc' : 'none'};
  justify-content: space-between;
  align-items: baseline;
  align-content: flex-start;
  flex-wrap: nowrap;
  font-family: ${p => p.theme.fontFamily.body};
`

export const StyledTitle = styled('div')<{}>`
  font-size: 14px;
  line-height: 1.3;
  color: #4b4c5c;
  flex: 1 1;
  padding: 10px 5px 10px 0;
`

export const StyledContentWrapper = styled('div')<{}>`
  flex: 1 1;
  text-align: right;
`

export const StyledLink = styled('a')<{}>`
  cursor: pointer;
  display: inline-block;
  color: ${p => p.theme.color.subtleActive};
  font-size: 13px;
  font-weight: normal;
  letter-spacing: 0;
`
