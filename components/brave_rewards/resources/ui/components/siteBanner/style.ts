/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'
import bg1Url from './assets/bg_bats.svg'
import bg2Url from './assets/bg_hearts.svg'

interface StyleProps {
  padding?: boolean
  bg?: string
  isTwitterTip?: boolean
}

export const StyledWrapper = styled<StyleProps, 'div'>('div')`
  overflow-y: scroll;
  height: auto;
  padding: 0px;
  font-family: Poppins, sans-serif;
`

export const StyledContentWrapper = styled<StyleProps, 'div'>('div')`
  display: flex;
  justify-content: space-between;
  align-items: stretch;
  align-content: flex-start;
  flex-wrap: nowrap;
  max-width: 1320px;
  margin: 0 auto;
`

export const StyledContent = styled<{}, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: calc(100% - 336px);
  background: #e9f0ff;
  display: flex;
  flex-wrap: wrap;
  align-items: flex-start;
  align-content: space-between;
`

export const StyledDonation = styled<StyleProps, 'div'>('div')`
  flex-basis: 336px;
  background: #696fdc;
  justify-content: space-between;
  display: flex;
  flex-direction: column;
`

export const StyledBanner = styled<StyleProps, 'div'>('div')`
  position: relative;
  min-width: 900px;
  background: #DBE3F3;
`

export const StyledBannerImage = styled<Partial<Props>, 'div'>('div')`
  font-size: 38px;
  font-weight: 600;
  line-height: 0.74;
  color: #d1d1db;
  min-height: 176px;
  overflow: hidden;
  background: ${p => p.bgImage
    ? `url(${p.bgImage}) no-repeat top center / cover`
    : `url(${bg1Url}) no-repeat top left, url(${bg2Url}) no-repeat top right, #9e9fab`
  };
`

// 900:176 is the aspect ratio for banner images.
export const StyledBannerFiller = styled<{}, 'div'>('div')`
  padding-top: calc(176 / 900 * 100%);
`

export const StyledClose = styled<{}, 'button'>('button')`
  top: 16px;
  right: 16px;
  position: absolute;
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
  width: 24px;
  height: 24px;
  color: #FFF;
  filter: drop-shadow(0px 0px 2px rgba(0, 0, 0, .4));
`

export const StyledLogoWrapper = styled<StyleProps, 'div'>('div')`
  padding-left: 45px;
  flex-basis: 217px;
  position: relative;
`

export const StyledLogoText = styled<StyleProps, 'div'>('div')`
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
  margin-top: 0px;
`

export const StyledLogoBorder = styled<StyleProps, 'div'>('div')`
  border: 6px solid #fff;
  border-radius: 50%;
  width: 160px;
  height: 160px;
  background: ${p => p.bg || '#DE4D26'};
  padding-top: ${p => p.padding ? '35px' : 0};
  margin: -66px 0 25px;
  overflow: hidden;
`

export const StyledTextWrapper = styled<StyleProps, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: calc(100% - 217px);
`

export const StyledTitle = styled<StyleProps, 'div'>('div')`
  font-size: ${p => p.isTwitterTip ? 18 : 28}px;
  font-weight: 600;
  line-height: 1;
  color: #4b4c5c;
  margin: 10px 0 0;
  padding-left: 0px;
`

export const StyledText = styled<StyleProps, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 1.5;
  color: #686978;
  padding-right: 37px;
  padding-left: 0px;
`

export const StyledWallet = styled<StyleProps, 'div'>('div')`
  font-size: 12px;
  color: #afb2f1;
  text-align: right;
  margin: 8px 0 10px;
  padding: 0 19px 0 55px;
`

export const StyledTokens = styled<{}, 'span'>('span')`
  color: #fff;
`

export const StyledOption = styled<{}, 'span'>('span')`
  color: rgba(255, 255, 255, 0.65);
`

export const StyledCenter = styled<{}, 'div'>('div')`
  max-width: 1024px;
  padding: 126px 0 0 238px;
  margin: 0 auto;
  user-select: none;
`

export const StyledUserName = styled<{}, 'div'>('div')`
  font-size: 38px;
  color: #fff;
  max-width: 1024px;
  padding: 70px 0 0 238px;
  margin: 0 auto;
  user-select: none;
`

export const StyledScreenName = styled<{}, 'div'>('div')`
  font-size: 24px;
  font-weight: 400;
  color: #000;
  max-width: 1024px;
  padding: 10px 0 0 238px;
  margin: 10px auto;
  user-select: none;
`

export const StyledSocialItem = styled<{}, 'a'>('a')`
  font-size: 12px;
  line-height: 2.5;
  letter-spacing: 0.2px;
  color: #9e9fab;
  text-decoration: none;
  display: inline-block;
  margin: 0 8px;
`

export const StyledSocialIcon = styled<{}, 'span'>('span')`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
  width: 22px;
  height: 22px;
`

export const StyledSocialWrapper = styled<StyleProps, 'div'>('div')`
  text-align: right;
  padding-right: 40px;
  margin-top: 15px;
`

export const StyledEmptyBox = styled<{}, 'div'>('div')`
  width: 100%;
  height: 39px;
`

export const StyledLogoImage = styled<StyleProps, 'div'>('div')`
  width: 148px;
  height: 148px;
  background: url(${p => p.bg}) no-repeat;
  background-size: cover;
`

export const StyledCheckbox = styled<StyleProps, 'div'>('div')`
  width: 180px;
  padding-left: 0px;
  margin: 15px 0 5px;
`
export const StyledNoticeWrapper = styled<{}, 'div'>('div')`
  background: #fff;
  border: 1px solid rgba(155, 157, 192, 0);
  border-radius: 4px;
  width: 90%;
  margin: 10px 0 18px;
  padding: 7px 15px;
  display: flex;
`
export const StyledNoticeIcon = styled<{}, 'div'>('div')`
  width: 39px;
  height: 39px;
  color: #00AEFF;
  margin: -2px 6px 0 0;
`
export const StyledNoticeText = styled<{}, 'div'>('div')`
  flex: 1;
  color: #67667D;
  font-size: 12px;
  font-family: ${p => p.theme.fontFamily.body};
  letter-spacing: 0;
  line-height: 18px;
`
export const StyledNoticeLink = styled<{}, 'a'>('a')`
  color: #0095FF;
  font-family: ${p => p.theme.fontFamily.body};
  font-weight: bold;
  display: inline-block;
  text-decoration: none;
  margin-left: 4px;
`
export const StyledVerifiedIcon = styled<{}, 'div'>('div')`
  position: absolute;
  top: -60px;
  right: 12px;
  width: 40px;
  height: 40px;
  color: ${p => p.theme.palette.blurple500};
  background-color: ${p => p.theme.palette.white};
  border-radius: 100%;
`
