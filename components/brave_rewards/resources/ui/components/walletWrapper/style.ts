/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Notification } from './'
import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
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

export const StyledWrapper = styled('div')<StyledProps>`
  overflow: hidden;
  font-family: ${p => p.theme.fontFamily.body};
  width: ${p => p.isMobile ? '100%' : '373px'};
  background: ${p => wrapperBackgroundRules(p.notification)};
  border-radius: ${p => p.compact ? '0' : '6px'};
  display: flex;
  flex-direction: column;
  max-width: 415px;
  margin: 0 auto;
  position: relative;
`

export const StyledHeader = styled('div')<{}>`
  padding: 16px 21px 0 19px;
  position: relative;
  z-index: 2;
`

export const StyledTitle = styled('div')<{}>`
  font-size: 16px;
  font-weight: bold;
  line-height: 1.38;
  letter-spacing: 0.5px;
  color: rgba(255, 255, 255, 0.65);
  margin-bottom: 30px;
`

export const StyledBalance = styled('div')<{}>`
  text-align: center;
`

export const StyledBalanceTokens = styled('div')<{}>`
  font-size: 36px;
  line-height: 0.61;
  letter-spacing: -0.4px;
  color: #fff;
  margin-top: 10px;
  font-weight: 300;
`

export const StyledContent = styled('div')<StyledProps>`
  padding: ${p => p.contentPadding ? '11px 25px 19px' : '0px'};
  background: #fff;
  flex: 1;
`

export const StyledAction = styled('button')<{}>`
  display: flex;
  background: none;
  padding: 4px;
  border: none;
  cursor: pointer;
  align-items: center;
  color: #A1A8F2;
`

export const StyledActionIcon = styled('div')<{}>`
  display: inline-block;
  width: 24px;
  height: 24px;
  margin-right: 6px;
  vertical-align: text-bottom;
`

export const StyledActionText = styled('div')<{}>`
  color: #fff;
  font-size: 14px;
  opacity: 0.65;
`

export const StyledCopy = styled('div')<StyledProps>`
  font-size: 12px;
  color: #838391;
  padding: 19px 15px;
  background: ${p => p.connected ? '#c4f2db' : '#dee2e6'};
  text-align: center;
`

export const StyledCopyImage = styled('span')<{}>`
  vertical-align: middle;
  display: inline-block;
  color: #838391;
  width: 27px;
  height: 27px;
`

export const StyledIconAction = styled('button')<{}>`
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

export const StyledBalanceConverted = styled('div')<{}>`
  font-family: Muli, sans-serif;
  font-size: 12px;
  line-height: 1.17;
  text-align: center;
  color: rgba(255, 255, 255, 0.65);
  margin: 8px 0;
  font-weight: 300;
`

export const StyledActionWrapper = styled('div')<{}>`
  text-align: center;
  font-size: 12px;
  color: #fff;
  display: flex;
  justify-content: space-evenly;
  margin: 15px 0 5px 0;
  padding-bottom: 3px;
`

export const StyledBalanceCurrency = styled('span')<{}>`
  text-transform: uppercase;
  opacity: 0.66;
  font-family: Muli, sans-serif;
  font-size: 16px;
  line-height: 0.88;
  color: #fff;
`

export const StyledCurve = styled('div')<StyledProps>`
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
    border: 20px solid ${p => p.background ? getRGB(p.background) : '#fff'};
  }
`

export const StyledAlertWrapper = styled('div')<{}>`
  display: flex;
  align-items: stretch;
  position: absolute;
  top: 0;
  left: 0;
  height: 100%;
  z-index: 5;
  width: 100%;
`

export const StyledAlertClose = styled('button')<{}>`
  position: absolute;
  background: none;
  border: none;
  padding: 0;
  top: 11px;
  right: 11px;
  cursor: pointer;
  width: 20px;
  height: 20px;
  color: #9E9FAB;
`

export const StyledBAT = styled('div')<{}>`
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

export const StyledNotificationIcon = styled('img')<StyledProps>`
  height: 48px;
  width: 48px;
  margin: 8px 0px 12px;
`

export const StyledNotificationCloseIcon = styled('div')<StyledProps>`
  height: 20px;
  width: 20px;
  position: absolute;
  top: 16px;
  right: 16px;
  color: #9E9FAB;
  cursor: pointer;
`

export const StyledNotificationContent = styled('div')<StyledProps>`
  display: block;
  text-align: center;
`

export const StyledNotificationMessage = styled('div')<StyledProps>`
  max-width: 285px;
  color: #4B4C5C;
  padding-bottom: 5px;
  margin: 0 auto;
`

export const StyledTypeText = styled('span')<StyledProps>`
  font-weight: 500;
  margin-right: 5px;
  display: inline-block;
`

export const StyledMessageText = styled('span')<StyledProps>`
  line-height: 20px;
  font-weight: 400;
  margin: 0px 5px;
  font-family: Muli, sans-serif;
`

export const StyledDateText = styled('span')<StyledProps>`
  font-weight: 400;
  margin-left: 5px;
  display: inline-block;
  color: #838391;
  font-family: Muli, sans-serif;
`

export const StyledButtonWrapper = styled('div')<StyledProps>`
  margin: 12px 0 15px;
  display: flex;
  justify-content: center;
`

export const StyledButton = styled(Button as ComponentType<ButtonProps>)`
  padding-left: 27px;
  padding-right: 27px;
`

export const StyledPipe = styled('span')<StyledProps>`
  font-weight: 300;
`

export const StyledVerifiedButton = styled('button')<{}>`
  box-sizing: border-box;
  outline-color: transparent;
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  font-family: Poppins, sans-serif;
  cursor: pointer;
  user-select: none;
  font-size: 11px;
  border-radius: 28px;
  min-width: 88px;
  padding: 7px 10px;
  color: #fff;
  background: inherit;
  border: none;

  .icon {
    padding-top: 1px;
  }
`

export const StyledVerifiedButtonText = styled('div')<{}>`
  /* min-height so that we get consistent height with / without an icon */
  min-height: 14px;
  display: flex;
  align-items: center;
  text-align: center;
  letter-spacing: 0;
  font-weight: 500;
  line-height: 1;
`

export const StyledVerifiedButtonIcon = styled('div')<{position: string}>`
  display: block;
  line-height: 0;
  height: 14px;
  width: 14px;
  margin: ${(p) => p.position === 'before' ? '0 6px 0 -4px' : '0 -4px 0 6px'};
`

export const StyledTextIcon = styled('div')<{}>`
  line-height: initial;
  background: ${palette.blurple600};
  width: 16px;
  height: 16px;
  border-radius: 8px;
  margin: 0 10px;
`

export const StyledDialogList = styled('ul')<{}>`
  list-style-position: inside;
  padding-left: 0;
  margin: 0;
  line-height: 150%;
`

export const StyledLink = styled('a')<{}>`
  color: ${palette.blue400};
  font-weight: bold;
  text-decoration: none;
  display: inline-block;
  cursor: pointer;
`
