/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'
import { setTheme } from '../../../helpers'

export const StyledWrapper = styled.div`
  position: ${(p: Props) => setTheme(p.theme, 'position') || 'relative'};
  top: ${(p: Props) => setTheme(p.theme, 'top') || 0};
  left: ${(p: Props) => setTheme(p.theme, 'left') || 0};
  display: flex;
  justify-content: flex-start;
  align-content: flex-start;
  flex-wrap: nowrap;
  background-color: #fff;
  box-shadow: 0 1px 0 0 #dfdfe8;
  padding: 30px 38px 33px 19px;
  align-items: center;
  font-family: Poppins, sans-serif;
` as any

export const StyledIcon = styled.span`
  width: 40px;
  height: 40px;
  flex-basis: 40px;
` as any

export const StyledContent = styled.div`
  flex-grow: 1;
  flex-basis: 50%;
  padding-left: 11px;
  font-size: 14px;
  line-height: 1.57;
  color: #838391;

  b {
    font-weight: 600;
    color: #4b4c5c;
  }
` as any

export const StyledClose = styled.div`
  width: 11px;
  height: 11px;
  position: absolute;
  top: 14px;
  right: 14px;
  z-index: 2;
` as any
