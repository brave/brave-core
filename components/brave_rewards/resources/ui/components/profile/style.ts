/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import palette from '../../../theme/colors'
import { Props, Provider } from './index'

const getOverflowRules = (provider?: Provider) => {
  if (provider) {
    return null
  }

  return css`
    text-overflow: ellipsis;
    overflow: hidden;
    white-space: nowrap;
  `
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  position: relative;
  display: flex;
  justify-content: space-between;
  align-items: center;
  align-content: flex-start;
  flex-wrap: nowrap;
  font-family: Poppins, sans-serif;
`

export const StyledImageWrapper = styled<Partial<Props>, 'div'>('div')`
  flex-basis: 30px;
  position: relative;

  ${p => p.type === 'big'
    ? css`
      height: 48px;
    `
    : ''
  };

  ${p => p.type !== 'big'
    ? css`
      height: 24px;
    `
    : ''
  };
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
  top: -5px;
  right: -8px;
  width: 20px;
  height: 20px;
  color: #392DD1;
  background-color: #FFFFFF;
  border-radius: 20px;
  padding: 1px;
`

export const StyledContent = styled<Partial<Props>, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 50%;
  margin-top: -5px;
  padding-left: ${p => p.type === 'big' ? '11px' : 0};
`

export const StyledTitleWrap = styled<Partial<Props>, 'div'>('div')`
  ${p => getOverflowRules(p.provider)}
  max-width: ${p => p.tableCell ? 235 : 260}px;
  margin-top: ${p => p.type === 'big' ? 2 : 0}px;
  margin-left: ${p => p.type !== 'big' ? 10 : 0}px;
`

export const StyledTitle = styled<Partial<Props>, 'span'>('span')`
  ${p => p.type === 'big'
    ? css`
      font-size: 18px;
      font-weight: 500;
      line-height: 1.22;
      letter-spacing: -0.2px;
      color: #4B4C5C;
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
  padding-left: 5px;

  ${p => p.type === 'big'
    ? css`
      font-weight: 300;
      font-size: 18px;
      letter-spacing: -0.2px;
    `
    : ''
  };

  color: ${p => p.type === 'big' ? '#4b4c5c' : '#b8b9c4'};
`

export const StyledProviderWrap = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  margin-bottom: -4px;
`

export const StyledVerifiedText = styled<{}, 'span'>('span')`
  font-size: 12px;
  color: #838391;
  font-weight: 400;
  letter-spacing: 0;
  margin-left: 4px;
`

export const StyledRefresh = styled<{}, 'span'>('span')`
  color: ${palette.blue400};
  line-height: 0;
  height: 18px;
  width: 18px;
  vertical-align: bottom;
  margin-left: auto;
`

export const StyledVerifiedCheckLink = styled<{}, 'span'>('span')`
  font-size: 12px;
  color: ${palette.blue400};
  cursor: pointer;
  text-decoration: none;
  margin-left: auto;
  z-index: 1;
`

export const StyledInlineVerified = styled<{}, 'span'>('span')`
  width: 19px;
  padding-top: 2px;
  margin-left: -2px;
  color: #392DD1;
`

export const StyledInlineUnVerified = styled<{}, 'span'>('span')`
  width: 19px;
  padding-top: 2px;
  margin-left: -2px;
  color: #D0D4D9;
`

export const StyledSubTitle = styled<{}, 'span'>('span')`
  margin-top: 5px;
`
