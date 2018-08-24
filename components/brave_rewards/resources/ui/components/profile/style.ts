/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { Props } from './index'

export const StyledWrapper = styled<{}, 'div'>('div')`
  position: relative;
  display: flex;
  justify-content: space-between;
  align-items: center;
  align-content: flex-start;
  flex-wrap: nowrap;
  font-family: Poppins, sans-serif;
`

export const StyledImageWrapper = styled<{}, 'div'>('div')`
  flex-basis: 30px;
  position: relative;
`

export const StyledImage = styled<Partial<Props>, 'img'>('img')`
  border-radius: 50%;

  ${p => p.type === 'big'
    ? css`
      width: 48px;
      height: 48px;
    `
    : ''
  };

  ${p => p.type !== 'big'
    ? css`
      width: 24px;
      height: 24px;
    `
    : ''
  };
`

export const StyledVerified = styled<{}, 'span'>('span')`
  position: absolute;
  bottom: 2px;
  right: -1px;
`

export const StyledContent = styled<Partial<Props>, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 50%;
  margin-top: -5px;
  padding-left: ${p => p.type === 'big' ? '11px' : 0};
`

export const StyledTitleWrap = styled<Partial<Props>, 'div'>('div')`
  margin-left: ${p => p.type !== 'big' ? '10px' : 0};
`

export const StyledTitle = styled<Partial<Props>, 'span'>('span')`
  white-space: nowrap;

  ${p => p.type === 'big'
    ? css`
      font-size: 18px;
      font-weight: 500;
      line-height: 1.22;
      letter-spacing: -0.2px;
      color: #4c54d2;
    `
    : ''
  };

  ${p => p.type !== 'big'
    ? css`
      font-family: Muli, sans-serif;
      font-size: 14px;
      font-weight: 600;
      line-height: 1.29;
      letter-spacing: -0.1px;
      color: #686978;
    `
    : ''
  };
`

export const StyledProvider = styled<Partial<Props>, 'span'>('span')`
  white-space: nowrap;
  padding-left: 5px;

  ${p => p.type === 'big'
    ? css`
      font-weight: 300;
      font-size: 18px;
    `
    : ''
  };

  color: ${p => p.type === 'big' ? '#4b4c5c' : '#b8b9c4'};
`

export const StyledProviderWrap = styled<{}, 'div'>('div')`
  font-size: 13px;
  color: #838391;
  margin-top: 3px;
`

export const StyledInlineVerified = styled<{}, 'span'>('span')`
  display: inline-block;
  vertical-align: middle;
`
