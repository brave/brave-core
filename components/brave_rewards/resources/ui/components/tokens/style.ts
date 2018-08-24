/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
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

const colors: Record<Type, string> = {
  contribute: '#9752cb',
  default: '#4b4c5c',
  donation: '#696fdc',
  earnings: '#c12d7c',
  notPaid: '#838391'
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

export const StyledWrapper = styled<Partial<Props>, 'div'>('div')`
  ${getStyle}
`

export const StyledTokens = styled<{}, 'span'>('span')`
  font-family: Poppins, sans-serif;
  font-weight: 300;
  line-height: 1.4;
  color: #686978;
  display: inline-block;
`

export const StyledTokenValue = styled<{}, 'span'>('span')`
  color: var(--tokens-value-color);
  font-size: var(--tokens-tokenNum-size);
  font-weight: 500;
`

export const StyledContent = styled<{}, 'span'>('span')`
  color: #9E9FAB;
  font-size: var(--tokens-text-size);
  font-family: Muli, sans-serif;
  line-height: 1.29;
  display: inline-block;
  margin-left: 10px;
`

export const StyledTokenCurrency = styled<{}, 'span'>('span')`
  font-size: var(--tokens-token-size);
  font-weight: 300;
  display: inline-block;
  margin-left: 4px;
`
