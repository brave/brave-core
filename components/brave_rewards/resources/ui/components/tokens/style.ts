/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'
import { setTheme } from '../../../helpers'

export const StyledTokens = styled.span`
  font-family: Poppins;
  font-weight: 300;
  line-height: 1.4;
  color: ${(p: Props) => p.theme && p.theme.color && p.theme.color.token ? p.theme.color.token : '#4b4c5c'};
  display: ${(p: Props) => setTheme(p.theme, 'display') || 'inline-block'};
` as any

export const StyledTokenValue = styled.span`
  color: ${(p: Props) => p.theme && p.theme.color && p.theme.color.tokenNum ? p.theme.color.tokenNum : 'inherit'};
  font-size: ${(p: Props) => p.theme && p.theme.size && p.theme.size.tokenNum ? p.theme.size.tokenNum : '16px'};
  font-weight: 500;
` as any

export const StyledContent = styled.span`
  color: ${(p: Props) => p.theme && p.theme.color && p.theme.color.text ? p.theme.color.text : '#b8b9c4'};
  font-size: ${(p: Props) => p.theme && p.theme.size && p.theme.size.text ? p.theme.size.text : '12px'};
  font-family: Muli;
  line-height: 1.29;
  display: inline-block;
  margin-left: 10px;
` as any

export const StyledTokenCurrency = styled.span`
  font-size: ${(p: Props) => p.theme && p.theme.size && p.theme.size.token ? p.theme.size.token : '14px'};
  font-weight: 300;
  display: inline-block;
  margin-left: 4px;
` as any
