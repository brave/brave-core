/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { Type, Props } from './index'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  left?: boolean
  selected?: boolean
  type?: Type
}

export const RewardsTabWrapper = styled<{}, 'div'>('div')`
  display: flex;
  font-family: Poppins,sans-serif;
`

export const StyledSwitch = styled<{}, 'div'>('div')`
  position: relative;
  display: block;
  width: 100%;
  height: 43px;
  cursor: pointer;
`

export const StyledSlider = styled<{}, 'div'>('div')`
  width: 100%;
  height: 100%;
  background: #DFDFE8;
  border-radius: 21.5px 21.5px 21.5px 21.5px;
`

export const StyledBullet = styled<Props, 'div'>('div')`
  top: -17px;
  width: 50%;
  height: 37px;
  background: ${p => p.theme.color.primaryBackground};
  border-radius: 21.5px 21.5px 21.5px 21.5px;
  position: relative;
  transition: all .4s ease;
  transform: translate(calc(${p => p.tabIndexSelected === 0 ? 2 : 97}%), calc(-50% - 4px));
  box-shadow: 0 3px 3px rgba(0, 0, 0, 0.05);
`

export const StyledTab = styled<StyleProps, 'div'>('div')`
  width: 50%;
  display: block;
  height: 100%;
  float: ${p => p.left ? 'left' : 'right'};
`

export const StyledText = styled<StyleProps, 'div'>('div')`
  z-index: 9;
  position: relative;
  font-size: 14px;
  width: 70%;
  margin: 13px auto 0 auto;
  text-overflow: ellipsis;
  display: block;
  overflow: hidden;
  text-align: center;
  user-select: none;
  color: ${p => p.selected
    ? p.type === 'contribute' ? palette.blurple500 : p.theme.color.brandBrave
    : p.theme.color.subtleActive
  };
  font-weight: ${p => p.selected ? '500' : 'normal'};

  @media (max-width: 485px) {
    font-size: 12px;
    margin-top: 15px;
  }

  @media (max-width: 435px) {
    font-size: 10px;
    margin-top: 17px;
  }

  @media (max-width: 385px) {
    font-size: 9px;
    margin-top: 18px;
  }
`
