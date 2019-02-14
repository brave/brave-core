/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Notification } from './'
import styled from 'styled-components'
import Button, { Props as ButtonProps } from '../../../components/buttonsIndicators/button'
import { ComponentType } from 'react'

interface StyledProps {
  connected?: boolean,
  contentPadding?: boolean,
  compact?: boolean,
  background?: string,
  isMobile?: boolean,
  notification?: Notification | undefined
}

import panelBg from './assets/panel.svg'

const getRGB = (rgbColor: string) => {
  return `rgb(${rgbColor})`
}

const wrapperBackgroundRules = (notification: Notification | undefined) => {
  if (!notification) {
    return `url(${panelBg}) no-repeat top left,
    linear-gradient(172deg, #392dd1, rgba(255, 26, 26, 0.53)),
    linear-gradient(#7d7bdc, #7d7bdc)`
  }

  return 'linear-gradient(-180deg, rgba(255,255,255,1) 0%, rgba(228,242,255,1) 40%)'
}

export const StyledWrapper = styled<StyledProps, 'div'>('div')`
  overflow: hidden;
  box-shadow: 0 1px 12px 0 rgba(99,105,110,0.18);
  font-family: Poppins, sans-serif;
  width: ${p => p.isMobile ? '100%' : '373px'};
  background: ${p => wrapperBackgroundRules(p.notification)};
  min-height: ${p => p.compact ? 'unset' : '715px'};
  border-radius: ${p => p.compact ? '0' : '6px'};
  display: flex;
  flex-direction: column;
`

export const StyledHeader = styled<{}, 'div'>('div')`
  padding: 16px 21px 0 19px;
  position: relative;
`

export const StyledTitle = styled<{}, 'div'>('div')`
  font-size: 16px;
  font-weight: 500;
  line-height: 1.38;
  letter-spacing: -0.2px;
  color: rgba(255, 255, 255, 0.65);
`

export const StyledBalance = styled<{}, 'div'>('div')`
  text-align: center;
`

export const StyleGrantButton = styled<{}, 'div'>('div')`
  display: flex;
  justify-content: center;
`

export const StyledBalanceTokens = styled<{}, 'div'>('div')`
  font-size: 36px;
  line-height: 0.61;
  letter-spacing: -0.4px;
  color: #fff;
  margin-top: 10px;
  font-weight: 300;
`

export const StyledContent = styled<StyledProps, 'div'>('div')`
  padding: ${p => p.contentPadding ? '11px 25px 19px' : '0px'};
  position: relative;
  background: #f9fbfc;
  flex: 1;
  height: 381px;
`

export const StyledAction = styled<{}, 'button'>('button')`
  display: flex;
  background: none;
  padding: 4px;
  border: none;
  cursor: pointer;
  align-items: center;
  color: #A1A8F2;
`

export const StyledActionIcon = styled<{}, 'div'>('div')`
  display: inline-block;
  width: 24px;
  height: 24px;
  margin-right: 6px;
  vertical-align: text-bottom;
`

export const StyledActionText = styled<{}, 'div'>('div')`
  color: #fff;
  font-size: 14px;
  opacity: 0.65;
`

export const StyledCopy = styled<StyledProps, 'div'>('div')`
  font-size: 12px;
  color: #838391;
  padding: 19px 15px;
  background: ${p => p.connected ? '#dcdfff' : '#dee2e6'};
  text-align: center;
`

export const StyledCopyImage = styled<{}, 'span'>('span')`
  vertical-align: middle;
  display: inline-block;
  color: #838391;
  width: 27px;
  height: 27px;
`

export const StyledIconAction = styled<{}, 'button'>('button')`
  position: absolute;
  top: 15px;
  right: 21px;
  background: none;
  padding: 0;
  border: none;
  cursor: pointer;
  color: #A1A8F2;
  width: 24px;
  height: 24px;
`

export const StyledBalanceConverted = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 12px;
  line-height: 1.17;
  text-align: center;
  color: rgba(255, 255, 255, 0.65);
  margin: 8px 0;
  font-weight: 300;
`

export const StyledGrantWrapper = styled<{}, 'div'>('div')`
  margin-top: 13px;
`

export const StyledGrant = styled<{}, 'div'>('div')`
  font-family: Muli, sans-serif;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.60);
  text-align: center;
  margin-bottom: 3px;

  b {
    font-weight: 600;
    color: #fff;
    min-width: 81px;
    text-align: right;
    display: inline-block;
  }

  span {
    min-width: 135px;
    text-align: left;
    display: inline-block;
  }
`

export const StyledActionWrapper = styled<{}, 'div'>('div')`
  text-align: center;
  font-size: 12px;
  color: #fff;
  display: flex;
  justify-content: space-evenly;
  margin: 15px 0 5px 0;
  padding-bottom: 3px;
`

export const StyledBalanceCurrency = styled<{}, 'span'>('span')`
  text-transform: uppercase;
  opacity: 0.66;
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 0.88;
  color: #fff;
`

export const StyledCurve = styled<StyledProps, 'div'>('div')`
  padding: 10px 0;
  position: relative;
  overflow: hidden;
  margin: 0 -21px 0 -19px;
  z-index: 5;

  :before {
    content: "";
    position: absolute;
    bottom: -16px;
    margin-left: -50%;
    height: 240px;
    width: 200%;
    border-radius: 100%;
    border: 20px solid ${p => p.background ? getRGB(p.background) : '#f9fbfc'};
  }
`

export const StyledAlertWrapper = styled<{}, 'div'>('div')`
  display: flex;
  align-items: stretch;
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  z-index: 5;
  width: 100%;
`

export const StyledAlertClose = styled<{}, 'button'>('button')`
  position: absolute;
  background: none;
  border: none;
  padding: 0;
  top: 11px;
  right: 11px;
  cursor: pointer;
  width: 15px;
  height: 15px;
  color: #B8B9C4;
`

export const StyledBAT = styled<{}, 'div'>('div')`
  text-align: center;
  max-width: 300px;
  margin: 20px auto 0;
  color: #686978;

  a {
    color: #686978;

    &:hover {
      text-decoration: none;
    }
  }
`

export const StyledNotificationIcon = styled<StyledProps, 'img'>('img')`
  height: 48px;
  width: 48px;
  margin: 8px 0px 12px;
`

export const StyledNotificationCloseIcon = styled<StyledProps, 'div'>('div')`
  height: 20px;
  width: 20px;
  position: absolute;
  top: 16px;
  right: 16px;
  color: #9E9FAB;
  cursor: pointer;
`

export const StyledNotificationContent = styled<StyledProps, 'div'>('div')`
  display: block;
  text-align: center;
`

export const StyledNotificationMessage = styled<StyledProps, 'div'>('div')`
  max-width: 285px;
  color: #4B4C5C;
  padding-bottom: 5px;
  margin: 0 auto;
`

export const StyledTypeText = styled<StyledProps, 'span'>('span')`
  font-weight: 500;
  margin-right: 5px;
  display: inline-block;
`

export const StyledMessageText = styled<StyledProps, 'span'>('span')`
  line-height: 20px;
  font-weight: 400;
  margin: 0px 5px;
  font-family: Muli, sans-serif;
`

export const StyledDateText = styled<StyledProps, 'span'>('span')`
  font-weight: 400;
  margin-left: 5px;
  display: inline-block;
  color: #838391;
  font-family: Muli, sans-serif;
`

export const StyledButtonWrapper = styled<StyledProps, 'div'>('div')`
  margin: 12px 0 15px;
  display: flex;
  justify-content: center;
`

export const StyledButton = styled(Button as ComponentType<ButtonProps>)`
  padding-left: 27px;
  padding-right: 27px;
`

export const StyledPipe = styled<StyledProps, 'span'>('span')`
  font-weight: 300;
`
