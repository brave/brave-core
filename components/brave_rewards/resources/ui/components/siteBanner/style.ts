/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Props } from './index'

const bg1 = require('./assets/bg_bats.svg')
const bg2 = require('./assets/bg_hearts.svg')

interface StyleProps {
  padding: boolean
  bg?: string
}

export const StyledWrapper = styled<{}, 'div'>('div')`
  background-color: rgba(12, 13, 33, 0.85);
  position: fixed;
  top: 0;
  left: 0;
  height: 100vh;
  width: 100%;
  font-family: Poppins, sans-serif;
`

export const StyledContentWrapper = styled<{}, 'div'>('div')`
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

export const StyledDonation = styled<{}, 'div'>('div')`
  flex-basis: 336px;
  background: #696fdc;
  justify-content: space-between;
  display: flex;
  flex-direction: column;
`

export const StyledBanner = styled<{}, 'div'>('div')`
  position: relative;
  min-width: 900px;
  background: #DBE3F3;
`

export const StyledBannerImage = styled<Partial<Props>, 'div'>('div')`
  font-size: ${p => !p.bgImage ? '38px' : 0};
  font-weight: 600;
  line-height: 0.74;
  color: #d1d1db;
  height: 176px;
  background: ${p => p.bgImage
    ? `url(${p.bgImage}) no-repeat top center / cover`
    : `url(${bg1}) no-repeat top left, url(${bg2}) no-repeat top right, #9e9fab`
  };
`

export const StyledClose = styled<{}, 'button'>('button')`
  top: 14px;
  right: 17px;
  position: absolute;
  background: none;
  border: none;
  padding: 0;
  cursor: pointer;
`

export const StyledLogoWrapper = styled<{}, 'div'>('div')`
  padding-left: 45px;
  flex-basis: 217px;
`

export const StyledLogoText = styled<{}, 'div'>('div')`
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

export const StyledTextWrapper = styled<{}, 'div'>('div')`
  flex-grow: 1;
  flex-shrink: 1;
  flex-basis: calc(100% - 217px);
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 28px;
  font-weight: 600;
  line-height: 1;
  color: #4b4c5c;
  margin: 41px 0 0;
`

export const StyledText = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 1.5;
  color: #686978;
  padding-right: 37px;
`

export const StyledRecurring = styled<{}, 'div'>('div')`
  flex-basis: 100%;
  font-size: 14px;
  line-height: 2;
  color: #9e9fab;
  background: #fff;
  height: 56px;
  padding: 13px 0 0 45px;
`

export const StyledRemove = styled<{}, 'span'>('span')`
  font-family: Muli, sans-serif;
  font-size: 14px;
  line-height: 1.29;
  color: #d1d1db;
  display: inline-block;
  margin-left: 15px;
`

export const StyledWallet = styled<{}, 'div'>('div')`
  font-size: 12px;
  color: #afb2f1;
  text-align: right;
  margin: 8px 0 10px;
  padding: 0 19px 0 55px;
`

export const StyledTokens = styled<{}, 'span'>('span')`
  color: #fff;
`

export const StyledIconRecurring = styled<{}, 'span'>('span')`
  display: inline-block;
  margin-left: 6px;
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

export const StyledIconRecurringBig = styled<{}, 'span'>('span')`
  vertical-align: top;
  display: inline-block;
  margin-right: 5px;
`

export const StyledIconRemove = styled<{}, 'span'>('span')`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
`

export const StyledSocialItem = styled<{}, 'a'>('a')`
  font-size: 12px;
  line-height: 2.5;
  letter-spacing: 0.2px;
  color: #9e9fab;
  display: block;
  text-decoration: none;
`

export const StyledSocialIcon = styled<{}, 'span'>('span')`
  vertical-align: middle;
  display: inline-block;
  margin-right: 5px;
`
