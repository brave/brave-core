/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { Props } from './index'

const getBgColor = (p: Props) => {
  let color = '#fff'

  if (p.bg) {
    switch (p.type) {
      case 'error':
        color = '#FFEEF1'
        break
      case 'success':
        color = '#E7F6FF'
        break
      case 'warning':
        color = '#FAF2DE'
        break
    }
  }

  return css`
    --alert-wrapper-color: ${color};
  `
}

const getColor = (p: Props) => {
  let color = '#838391'
  let bold = '#4b4c5c'

  if (p.colored) {
    switch (p.type) {
      case 'error':
        color = bold = '#F36980'
        break
      case 'success':
        color = bold = '#67D79D'
        break
      case 'warning':
        color = bold = '#FF7900'
        break
    }
  }

  return css`
    --alert-content-color: ${color};
    --alert-content-bold: ${bold};
  `
}

export const StyledWrapper = styled<Props, 'div'>('div')`
  height: 100%;
  display: flex;
  justify-content: flex-start;
  align-content: flex-start;
  align-items: center;
  flex-wrap: nowrap;
  padding: 15px 38px 15px 19px;
  font-family: Poppins, sans-serif;
  width: 100%;
  ${getBgColor};
  background: var(--alert-wrapper-color);
`

export const StyledIcon = styled<{}, 'span'>('span')`
  width: 40px;
  height: 40px;
  flex-basis: 40px;
`

export const StyledContent = styled<Props, 'div'>('div')`
  flex-grow: 1;
  flex-basis: 50%;
  padding-left: 19px;
  font-family: Muli, sans-serif;
  font-size: 16px;
  font-weight: 300;
  letter-spacing: -0.3px;
  ${getColor};
  color: var(--alert-content-color);

  b {
    font-weight: 600;
    color: var(--alert-content-bold);
  }
`

export const StyledClose = styled<{}, 'div'>('div')`
  width: 11px;
  height: 11px;
  position: absolute;
  top: 14px;
  right: 14px;
  z-index: 2;
`
