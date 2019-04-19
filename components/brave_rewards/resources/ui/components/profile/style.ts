/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from '../../../theme'
import { Props } from './index'

export const StyledWrapper = styled<{}, 'div'>('div')`
  position: relative;
  display: flex;
  align-items: center;
  font-family: ${p => p.theme.fontFamily.body};
`

export const StyledImageWrapper = styled<Partial<Props>, 'div'>('div')`
  position: relative;
  display: flex;

  ${p => p.type === 'big'
    ? css`
      height: 32px;
      width: 32px;
    `
    : css`
    height: 24px;
    width: 24px
  `
  };
`

/* Fill container height/width without setting explicit size on img */
export const StyledImage = styled.img`
  max-width: 100%;
  max-height: 100%;
`

export const StyledVerified = styled<{}, 'span'>('span')`
  top: -6px;
  right: -8px;
  width: 16px;
  height: 16px;
  color: ${p => p.theme.palette.blurple500};
  background-color: #FFFFFF;
  border-radius: 20px;
  position: absolute;
`

export const StyledContent = styled.div`
  padding: 0 0 0 12px;
`

export const StyledTitleWrap = styled.div`
  display: flex;
  font-size: 14px;
  font-weight: 700;
  color: ${p => p.theme.palette.grey800};
`

export const StyledTitle = styled<Partial<Props>, 'span'>('span')`
  font-size: ${p => p.type === 'big' ? '18px' : null};
`

export const StyledProvider = styled<Partial<Props>, 'span'>('span')`
  padding-left: 4px;
  font-size: ${p => p.type === 'big' ? '16px' : null};
  font-weight: ${p => p.type === 'big' ? '400' : null};
`

export const StyledProviderWrap = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  font-size: 13px;
  margin: 4px 0;
`

export const StyledVerifiedText = styled<{}, 'span'>('span')`
  color: ${p => p.theme.palette.grey800};
`

export const StyledVerifiedCheckLink = styled<{}, 'span'>('span')`
  color: ${p => p.theme.palette.blurple500};
  cursor: pointer;
  text-decoration: none;
  z-index: 1;

  &:hover{
    text-decoration: underline;
    color: ${p => p.theme.palette.blurple400};
  }
`

export const StyledVerifiedDivider = styled.span`
  margin: 0 8px;
  height: 12px;
  width: 1px;
  background: ${p => p.theme.palette.grey400};
`

export const StyledInlineVerified = styled.span`
  color: ${p => p.theme.palette.blurple500};
  width: 16px;
  height: 16px;
  margin: 0 4px 0 0;
`

export const StyledInlineUnVerified = styled(StyledInlineVerified)`
  color: ${p => p.theme.palette.grey500};
`

export const StyledRefresh = styled(StyledInlineVerified)`
  display: flex;
`

export const StyledSubTitle = styled.span`
  margin-top: 4px;
`
