/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import styled, { css } from 'styled-components'
import { Props } from './index'

export const StyledCheckbox = styled('input')<{}>`
  -webkit-appearance: none;
  position: absolute;
  z-index: 99999999;
  width: 100%;
  height: 100%;
  top: 0;
  left: 0;

  &:active {
    outline: none;
  }

  &:focus {
    outline-style: solid;
    outline-color: ${p => p.theme.color.brandBrave};
    outline-width: 1px;
    outline-offset: -1px;
  }
`

export const StyledWrapper = styled('div')<Props>`
  box-sizing: border-box;
  display: flex;
  position: relative;
  height: 100%;
  justify-content: center;
  align-items: center;
`

export const StyleToggle = styled('div')<Props>`
  box-sizing: border-box;
  position: relative;
  display: block;
  cursor: pointer;
  height: ${(p) => p.size === 'small' ? '16px' : '24px'};
  width: ${(p) => p.size === 'small' ? '28px' : '40px'};

  ${(p) => p.disabled
    ? css`
      pointer-events: none;
      animation: none;
    ` : ''
  };
`

export const StyledSlider = styled('label')<Props>`
  box-sizing: border-box;
  background: ${(p) => p.disabled ? 'rgba(246,246,250,0.1)' : '#C4C7C9'};
  height: ${(p) => p.size === 'small' ? '6px' : '8px'};
  margin-top: ${(p) => p.size === 'small' ? '5px' : '8px'};
  width: 100%;
  border-radius: 3px;
  display: block;
`

const transform = (p: Props) => {
  let x = p.size === 'small' ? '12px' : '20px'
  let y = p.size === 'small' ? '3px' : '4px'

  if (!p.checked) {
    x = '-1px'
  }

  return { x, y }
}

const transformBullet = (p: Props) => `${transform(p).x}, calc(-50% - ${transform(p).y})`

export const StyledBullet = styled('label')<Props>`
   box-sizing: border-box;
   position: relative;
   border-radius: 50%;
   transition: all .4s ease;
   transform: ${p => `translate(${transformBullet(p)})`};
   width: ${p => p.size === 'small' ? '16px' : '20px'};
   height: ${p => p.size === 'small' ? '16px' : '20px'};
   background-color: ${p => {
    if (p.disabled) return 'rgba(235,236,240,0.8)'
    return p.checked ? '#fb542b' : '#ebecf0'
   }};
   display: block;
   box-shadow: 0 3px 3px rgba(0,0,0,0.05);
 `
