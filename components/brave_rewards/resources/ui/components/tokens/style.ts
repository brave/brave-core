/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { Props, Size, Type } from './index'

const sizes: Record<Size, { token: string, tokenNum: string, text: string }> = {
  mini: {
    text: '14px',
    token: '14px',
    tokenNum: '14px'
  },
  small: {
    text: '14px',
    token: '14px',
    tokenNum: '14px'
  },
  normal: {
    text: '14px',
    token: '14px',
    tokenNum: '14px'
  }
}

const colors: Record<Type, string> = {
  contribute: '#696fdc',
  default: '#4b4c5c',
  earning: '#c12d7c'
}

const getStyle = (p: Partial<Props>) => {
  const size = sizes[p.size || 'normal']
  const color = colors[p.color || 'default']

  return css`
    --tokens-value-color: ${color};
    --tokens-text-size: ${size.text};
    --tokens-token-size: ${size.token};
    --tokens-tokenNum-size: ${size.tokenNum};
  `
}

export const StyledWrapper = styled('span')<Partial<Props>>`
  ${getStyle}
`

export const StyledTokens = styled('span')<{}>`
  display: inline-block;
`

export const StyledTokenValue = styled('span')<{}>`
  color: var(--tokens-value-color);
  font-size: var(--tokens-tokenNum-size);
`

export const StyledContent = styled('span')<{}>`
  color: #9E9FAB;
  font-size: var(--tokens-text-size);
  display: inline-block;
  margin-left: 8px;
`

export const StyledTokenCurrency = styled('span')<{}>`
  font-size: var(--tokens-token-size);
  display: inline-block;
  margin-left: 4px;
  text-transform: uppercase;
`
