/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'

export const StyledWrapper = styled.button`
  margin-bottom: 8px;
  user-select: none;
  font-family: Poppins, sans-serif;
  border: none;
  background: none;
  padding: 0;
  cursor: pointer;
` as any

export const StyledAmount = styled.span`
  opacity: 1;
  border-radius: 20px;
  color: #fff;
  border: 1px solid rgba(255, 255, 255, 0.35);
  background: ${(p: Props) => p.selected ? 'rgba(255, 255, 255, 0.35)' : 'transparent'};
  vertical-align: baseline;
  padding: ${(p: Props) => p.type === 'big' ? '10px 16px' : '7px 12px'};
  min-width: ${(p: Props) => p.type === 'big' ? '118px' : '69px'};
  font-size: 13px;
  font-weight: 600;
  display: inline-block;
  margin-right: 12px;
` as any

export const StyledTokens = styled.span`
  font-weight: 400;
` as any

export const StyledLogo = styled.span`
  vertical-align: text-bottom;
  margin-right: 6px;
` as any

export const StyledConverted = styled.span`
  vertical-align: baseline;
  opacity: ${(p: Props) => p.selected ? 1 : 0.4};
  font-size: ${(p: Props) => p.type === 'big' ? '12px' : '10px'};
  color: #ffffff;
  font-weight: 500;
` as any
