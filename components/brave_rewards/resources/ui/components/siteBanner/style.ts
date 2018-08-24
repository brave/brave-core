/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props, Theme } from './index'
import { setTheme } from '../../../helpers'

const bg1 = require('./assets/bg_bats.svg')
const bg2 = require('./assets/bg_hearts.svg')

export const StyledWrapper = styled.div`
  background-color: rgba(12, 13, 33, 0.85);
  position: fixed;
  top: 0;
  left: 0;
  height: 100vh;
  width: 100%;
  font-family: Poppins, sans-serif;
` as any

export const StyledContentWrapper = styled.div`
  display: flex;
  justify-content: space-between;
  align-items: stretch;
  align-content: flex-start;
  flex-wrap: nowrap;
  max-width: 1320px;
  margin: 0 auto;
` as any

export const StyledContent = styled.div`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: calc(100% - 336px);
  background: #e9f0ff;
  display: flex;
  flex-wrap: wrap;
  align-items: flex-start;
  align-content: space-between;
` as any

export const StyledDonation = styled.div`
  flex-basis: 336px;
  background: #696fdc;
  justify-content: space-between;
  display: flex;
  flex-direction: column;
` as any

export const StyledBanner = styled.div`
  position: relative;
  min-width: 900px;
  background: #DBE3F3;
` as any

export const StyledBannerImage = styled.div`
  font-size: ${(p: Props) => !p.bgImage ? '38px' : 0};
  font-weight: 600;
  line-height: 0.74;
  color: #d1d1db;
  height: 176px;
  background: ${(p: Props) => p.bgImage
    ? `url(${p.bgImage}) no-repeat top center / cover`
    : `url(${bg1}) no-repeat top left, url(${bg2}) no-repeat top right, #9e9fab`
  };
` as any

export const StyledClose = styled.button`
  top: 14px;
  right: 17px;
  position: absolute;
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
` as any

export const StyledLogoWrapper = styled.div`
  padding-left: 45px;
  flex-basis: 217px;
` as any

export const StyledLogoText = styled.div`
	background: inherit;
	-webkit-background-clip: text;
	color: transparent;
	filter: invert(1) grayscale(1) contrast(9);
  font-size: 80px;
  font-weight: 600;
  text-align: center;
  letter-spacing: 0;
  line-height: 1;
  text-transform: uppercase;
  user-select: none;
` as any

export const StyledLogoBorder = styled.div`
  border: 6px solid #fff;
  border-radius: 50%;
  width: 160px;
  height: 160px;
  background: ${(p: {customStyle: Theme, padding: boolean}) => setTheme(p.customStyle, 'logoBgColor') || '#DE4D26'};
  padding-top: ${(p: {customStyle: Theme, padding: boolean}) => p.padding ? '35px' : 0};
  margin: -66px 0 25px;
  overflow: hidden;
` as any

export const StyledTextWrapper = styled.div`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: calc(100% - 217px);
` as any

export const StyledTitle = styled.div`
  font-size: 28px;
  font-weight: 600;
  line-height: 1;
  color: #4b4c5c;
  margin: 41px 0 0;
` as any

export const StyledText = styled.div`
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 1.5;
  color: #686978;
  padding-right: 37px;
` as any

export const StyledRecurring = styled.div`
  flex-basis: 100%;
  font-size: 14px;
  line-height: 2;
  color: #9e9fab;
  background: #fff;
  height: 56px;
  padding: 13px 0 0 45px;
` as any

export const StyledRemove = styled.span`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1.29;
  color: #d1d1db;
  display: inline-block;
  margin-left: 15px;
` as any

export const StyledWallet = styled.div`
  font-size: 12px;
  color: #afb2f1;
  text-align: right;
  margin: 8px 0 10px;
  padding: 0 19px 0 55px;
` as any

export const StyledTokens = styled.span`
  color: #fff;
` as any

export const StyledIconRecurring = styled.span`
  display: inline-block;
  margin-left: 6px;
` as any

export const StyledOption = styled.span`
  color: rgba(255, 255, 255, 0.65);
` as any

export const StyledCenter = styled.div`
  max-width: 1024px;
  padding: 126px 0 0 238px;
  margin: 0 auto;
  user-select: none;
` as any

export const StyledIconRecurringBig = styled.span`
  vertical-align: top;
  display: inline-block;
  margin-right: 5px;
` as any

export const StyledIconRemove = styled.span`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
` as any

export const StyledSocialItem = styled.a`
  font-size: 12px;
  line-height: 2.5;
  letter-spacing: 0.2px;
  color: #9e9fab;
  display: block;
  text-decoration: none;
` as any

export const StyledSocialIcon = styled.span`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
` as any
