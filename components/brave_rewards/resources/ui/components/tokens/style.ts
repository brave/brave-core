/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props, Size, Type } from './index'

const sizes: Record<Size, {token: string, tokenNum: string, text: string}> = {
  small: {
    text: '10px',
    token: '12px',
    tokenNum: '14px'
  },
  normal: {
    text: '12px',
    token: '14px',
    tokenNum: '16px'
  }
}

const color: Record<Type, string> = {
  contribute: '#9752cb',
  default: '#4b4c5c',
  donation: '#696fdc',
  earnings: '#c12d7c',
  notPaid: '#838391'
}

export const StyledTokens = styled.span`
  font-family: Poppins, sans-serif;
  font-weight: 300;
  line-height: 1.4;
  color: #686978;
  display: inline-block;
` as any

export const StyledTokenValue = styled.span`
  color: ${(p: Props) => color[p.color || 'default']};
  font-size: ${(p: Props) => sizes[p.size || 'normal'].tokenNum};
  font-weight: 500;
` as any

export const StyledContent = styled.span`
  color: #9E9FAB;
  font-size: ${(p: Props) => sizes[p.size || 'normal'].text};
  font-family: Muli, sans-serif;
  line-height: 1.29;
  display: inline-block;
  margin-left: 10px;
` as any

export const StyledTokenCurrency = styled.span`
  font-size: ${(p: Props) => sizes[p.size || 'normal'].token};
  font-weight: 300;
  display: inline-block;
  margin-left: 4px;
` as any
