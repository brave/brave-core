/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { setTheme } from '../../../helpers'
import { Props } from './index'

export const StyledWrapper = styled.div`
  width: 100%;
  position: relative;
  height: auto;
  border-radius: 6px;
  background-color: #fff;
  box-shadow: 0 0 8px 0 rgba(99, 105, 110, 0.12);
  padding: 30px 36px;
  margin-bottom: 28px;
  font-family: Poppins;
` as any

export const StyledFlip = styled.div`
  display: flex;
  width: 200%;
` as any

export const StyledContentWrapper = styled.div`
  display: flex;
  height: ${(p: {open: boolean}) => p.open ? 'auto' : '0'};
  flex-basis: ${(p: {open: boolean}) => p.open ? '50%' : '0'};
  flex-wrap: wrap;
  overflow: hidden;
` as any

export const StyledLeft = styled.div`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: 50%;
` as any

export const StyledRight = styled.div`
  flex-basis: 40px;
` as any

export const StyledTitle = styled.div`
  height: 36px;
  font-size: 22px;
  font-weight: 600;
  line-height: 1.27;
  letter-spacing: normal;
  color: ${(p: Props) => {
    if (p.checked === false) return '#838391'
    return setTheme(p.theme, 'titleColor') || '#4b4c5c'
  }}
` as any

export const StyledBreak = styled.div`
  width: 100%;
  display: block;
` as any

export const StyledDescription = styled.div`
  width: 100%;
  padding-right: 20px;
  font-family: Muli;
  font-size: 14px;
  line-height: 1.29;
  letter-spacing: normal;
  color: #a4aeb8;
` as any

export const StyledSettingsIcon = styled.span`
  width: 20px;
  float: ${(p: {float: string}) => p.float ? p.float : 'none'};
  margin-top: 8px;
` as any

export const StyledContent = styled.div`
  flex-basis: 100%;
  flex-grow: 1;
  margin-top: 25px;
` as any

export const StyledSettingsWrapper = styled.div`
  background: #fff;
  overflow: hidden;
  height: ${(p: {open: boolean}) => p.open ? 'auto' : '0'};
  flex-basis: ${(p: {open: boolean}) => p.open ? '50%' : '0'};
` as any

export const StyledSettingsClose = styled.div`
  display: ${(p: {open: boolean}) => p.open ? 'block' : 'none'};
  position: absolute;
  right: 35px;
  top: 35px;
  width: 13px;
  height: 13px;
` as any

export const StyledSettingsTitle = styled.div`
  text-align: center;
  margin-bottom: 15px;
` as any

export const StyledSettingsText = styled.span`
  font-size: 16px;
  font-weight: 600;
  line-height: 1.75;
  color: #4b4c5c;
  position: relative;
  top: -4px;
  display: inline-block;
  margin-left: 20px;
` as any
