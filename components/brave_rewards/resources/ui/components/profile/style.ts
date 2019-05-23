/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from '../../../theme'
import { Props } from './index'
import { keyframes } from 'styled-components'

const fadeInOut = keyframes`
  0% {
    opacity: 0;
  }
  20% {
    opacity: 1;
  }
  80% {
    opacity: 1;
  }
  100% {
    opacity: 0;
  }
`

const fadeIn = keyframes`
  from {
    opacity: 0.2;
  }
  to {
    opacity: 1;
  }
`

const fadeOutWrap = keyframes`
  from {
    opacity: 1;
  }
  to {
    opacity: 0.2;
  }
`

const fadeInFromNull = keyframes`
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
`

const fadeOutToNull = keyframes`
  from {
    opacity: 1;
  }
  to {
    opacity: 0;
  }
`

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

export const StyledProviderWrapRefreshing = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  font-size: 13px;
  margin: 4px 0;
  opacity: 0.2;
  animation-name: ${fadeOutWrap};
  animation-duration: 500ms;
  animation-timing-function: ease-out;
  animation-fill-mode: forwards;
`

export const StyledProviderWrapRefreshFinished = styled<{}, 'div'>('div')`
  display: flex;
  align-items: center;
  font-size: 13px;
  margin: 4px 0;
  opacity: 0.2;
  animation-name: ${fadeIn};
  animation-duration: 500ms;
  animation-timing-function: ease-in;
  animation-fill-mode: forwards;
  animation-delay: 2250ms;
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
export const StyledVerifiedCheckNoLink = styled<{}, 'span'>('span')`
  color: ${p => p.theme.palette.grey400};
  text-decoration: none;
  z-index: 1;
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

export const StyledRefreshOverlay = styled<{}, 'div'>('div')`
  width: 100%;
  height: 100%;
  z-index: 1;
  position: absolute;
  display: flex;
  justify-content: center;
  color: ${p => p.theme.palette.blurple500};
  top: 0;
  left: 0;
  margin-top: 25px;
`

export const StyledRefreshOverlayFinished = styled<{}, 'div'>('div')`
  width: 100%;
  height: 100%;
  z-index: 2;
  position: absolute;
  display: flex;
  justify-content: center;
  color: ${p => p.theme.palette.blurple500};
  top: 0;
  left: 0;
  margin-top: 25px;
  opacity: 1;
  animation-name: ${fadeOutToNull};
  animation-duration: 500ms;
  animation-timing-function: ease-out;
  animation-fill-mode: forwards;
`

export const StyledRefreshCheckOverlayFinished = styled<{}, 'div'>('div')`
  width: 100%;
  height: 100%;
  z-index: 1;
  position: absolute;
  display: flex;
  justify-content: center;
  color: ${p => p.theme.palette.blurple500};
  top: 0;
  left: 0;
  margin-top: 25px;
  opacity: 0;
  animation-name: ${fadeInOut};
  animation-delay: 1000ms;
  animation-duration: 1250ms;
  animation-timing-function: ease-in-out;
  animation-fill-mode: forwards;
`

export const StyledRefresh = styled(StyledInlineVerified)`
  position: absolute;
  text-align: center;
  display: flex;
  opacity: 0;
  animation-name: ${fadeInFromNull};
  animation-duration: 500ms;
  animation-timing-function: ease-in;
  animation-fill-mode: forwards;
`

export const StyledRefreshLoaderFinished = styled(StyledInlineVerified)`
  position: absolute;
  text-align: center;
  display: flex;
`

export const StyledRefreshFinished = styled<{}, 'span'>('span')`
  position: absolute;
  height: 24px;
  width: 24px;
  color: ${p => p.theme.palette.blurple500};
`

export const StyledSubTitle = styled.span`
  margin-top: 4px;
`
