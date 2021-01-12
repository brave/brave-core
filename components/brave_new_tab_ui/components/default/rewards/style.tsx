// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled, { css } from 'brave-ui/theme'
import palette from 'brave-ui/theme/colors'
import confettiImageUrl from './confetti.png'
import Button, { Props as ButtonProps } from 'brave-ui/components/buttonsIndicators/button'
import TOSAndPP, { Props as TOSProps } from '../../../../brave_rewards/resources/ui/components/TOSAndPP'

interface StyleProps {
  isLast?: boolean
  isActionPrompt?: boolean
  isInTab?: boolean
  isNotification?: boolean
}

const Base = styled('div')`
  overflow-x: hidden;
  color: white;
  font-family: Muli, sans-serif;
  width: 284px;
`

export const WidgetWrapper = styled(Base)`
  position: relative;
  /* Show a 1x1 grid with all items overlapping.
      This makes sure that our layered notifications increase the height of the
      whole widget. Absolute positioning would not accomplish that. */
  display: grid;
  background: ${palette.neutral900};
  border-radius: 6px;
`

export const WidgetLayer = styled(Base)`
  padding: 16px 22px 22px 22px;
  grid-row: 1 / 2;
  grid-column: 1 / 2;
`

export const Footer = styled<{}, 'div'>('div')`
  max-width: 275px;
  margin-top: 25px;
`

export const BatIcon = styled<StyleProps, 'div'>('div')`
  width: ${p => p.isNotification ? 20 : 27}px;
  height: ${p => p.isNotification ? 20 : 27}px;
  margin-right: 8px;
`

export const RewardsTitle = styled<StyleProps, 'div'>('div')`
  margin-top: ${p => p.isInTab ? 6 : 0}px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  font-family: Poppins, sans-serif;
`

export const RewardsNotificationTitle = styled(RewardsTitle)`
  font-size: 15px;
  margin-bottom: 10px;
`

export const ServiceLink = styled<{}, 'a'>('a')`
  color: ${p => p.theme.color.brandBrave};
  font-weight: 600;
  text-decoration: none;
`

export const LearnMoreText = styled<{}, 'div'>('div')`
  font-size: 12px;
  margin-top: 34px;
`

export const Title = styled<{ isGrant?: boolean}, 'span'>('span')`
  font-size: ${p => p.isGrant ? 16 : 13}px;
  display: block;
  font-family: ${p => p.theme.fontFamily.heading};
  line-height: 1.5;
  font-weight: 500;
`

export const SubTitle = styled<{}, 'span'>('span')`
  font-size: 14px;
  display: block;
  margin-top: 15px;
  max-width: 250px;
  line-height: 1.4;
`

export const SubAction = styled(SubTitle)`
  margin: 15px 0;
`

export const SubTitleLink = styled<{}, 'a'>('a')`
  color: ${p => p.theme.color.brandBrave};
  text-decoration: none;
  cursor: pointer;
  &:hover {
    text-decoration: underline;
  }
`

export const TurnOnButton = styled<{}, 'button'>('button')`
  --rewards-widget-button-extra-space: 2px;
  margin: 0 auto;
  border: solid 1px ${palette.grey400};
  border-radius: 20px;
  background: transparent;
  padding: calc(8px + var(--rewards-widget-button-extra-space)) 20px;
  color: ${palette.neutral000};
  font-weight: 700;
  font-size: 16px;
  cursor: pointer;
  word-break: break-word;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  :focus {
    outline: none;
    box-shadow: 0 0 0 1px ${p => p.theme.color.brandBrave};
  }
`

const ActionButton = styled(TurnOnButton)`
  --rewards-widget-button-extra-space: 0px;
  margin: 0 auto;
  border: none;
  background: ${palette.blurple500};
  color: #fff;
  font-weight: 700;
  :hover {
    background: ${palette.blurple600};
  }
`

const StyledButtonIcon = styled('svg')`
  margin-right: 6px;
`

interface CoinsButtonProps {
  onClick?: () => void
  className?: string
}

const CoinsButton: React.FunctionComponent<CoinsButtonProps> = ({ onClick, className, children }) => {
  return (
    <ActionButton className={className} onClick={onClick}>
      <StyledButtonIcon xmlns='http://www.w3.org/2000/svg' width='18' height='12'>
        <path
          fill='#FFF'
          fillRule='evenodd'
          d='M12 12a6 6 0 01-1.491-.2c-.483.126-.988.2-1.51.2a5.999 5.999 0 01-1.49-.2c-.483.127-.987.2-1.509.2-3.308 0-6-2.692-6-6s2.692-6 6-6c.521 0 1.025.074 1.508.2.486-.125.984-.2 1.491-.2.522 0 1.025.074 1.508.2.486-.125.985-.2 1.492-.2C15.31 0 18 2.692 18 6s-2.692 6-6 6zM6 1.5A4.505 4.505 0 001.5 6c0 2.481 2.018 4.5 4.5 4.5s4.5-2.019 4.5-4.5S8.483 1.5 6 1.5zm4.136.163A5.976 5.976 0 0112 6a5.976 5.976 0 01-1.865 4.336C12.066 9.83 13.5 8.086 13.5 6c0-2.087-1.435-3.83-3.364-4.337zm2.999 0A5.978 5.978 0 0115 6a5.978 5.978 0 01-1.865 4.337C15.065 9.83 16.5 8.087 16.5 6c0-2.087-1.435-3.83-3.365-4.337zM6 5.25c1.262 0 2.25.823 2.25 1.875 0 .83-.62 1.511-1.5 1.764V9a.75.75 0 11-1.5 0H4.5a.75.75 0 110-1.5H6c.465 0 .75-.243.75-.375S6.465 6.75 6 6.75c-1.262 0-2.25-.824-2.25-1.876 0-.83.62-1.51 1.5-1.763V3a.75.75 0 111.5 0h.75a.75.75 0 110 1.5H6c-.465 0-.75.242-.75.374 0 .133.285.376.75.376z'
        />
      </StyledButtonIcon>
      {children}
    </ActionButton>
  )
}

export const TurnOnAdsButton = styled(Button as React.ComponentType<ButtonProps>)`
  margin: 15px 0;
  display: inline-block;
`

export const StartRewardsButton = styled<{}, 'button'>('button')`
  margin: 15px 0;
  display: inline-block;
  background: inherit;
  border: 1px solid #fff;
  width: 100%;
  text-align: center;
  border-radius: 20px;
  font-weight: bold;
  padding: 12px 0;
  font-size: 13px;
  cursor: pointer;
`

export const NotificationButton = styled(CoinsButton)`
  margin-top: 25px;
`

export const AmountItem = styled<StyleProps, 'div'>('div')`
  margin-top: 18px;
  margin-bottom: ${p => p.isLast ? -10 : 0}px;
  ${p => p.isActionPrompt && css`
    text-align: center;
    padding-bottom: 16px;
  `}
`

export const Amount = styled<{}, 'span'>('span')`
  font-size: 32px;
  margin-right: 10px;
  font-family: Poppins, sans-serif;
`

export const ConvertedAmount = styled<{}, 'span'>('span')`
  font-size: 14px;
`

export const AmountDescription = styled<{}, 'div'>('div')`
  font-size: 14px;
  color: ${palette.grey300};
`

export const AmountUSD = styled('span')`
  margin-left: 12px;
`

export const NotificationsList = styled('div')`
  grid-row: 1 / 2;
  grid-column: 1 / 2;
  display: grid;
  margin-top: 10px;
`

const BaseNotificationWrapper = styled(WidgetLayer)<{isGrant?: boolean}>`
  background-color: ${palette.neutral800};
  ${p => p.isGrant && css`
    background-image: url(${confettiImageUrl});
    background-position: bottom center;
    background-size: contain;
    background-repeat: no-repeat;
  `}
  border-radius: 6px;
`

export const NotificationWrapper = styled(BaseNotificationWrapper)`
  /* Stack the notifications offset from each other
  and make sure they all cover the panel */
  --ntp-rewards-notification-offset: calc(var(--notification-counter, 0) * 48px);
  margin-top: var(--ntp-rewards-notification-offset);
  min-height: calc(100% - var(--ntp-rewards-notification-offset));
  width: 100%;
  box-shadow: 0px 0px 16px 0px rgba(0, 0, 0, 0.5);
`

export const OrphanedNotificationWrapper = styled(BaseNotificationWrapper)`
  position: absolute;
  bottom: 100%;
  background-color: #339AF0;
  color: #fff;
  padding: 15px 20px;

  &:after {
    content: "";
    position: absolute;
    top: 100%;
    left: 20px;
    border-top: 20px solid black;
    border-top-color: inherit;
    border-left: 20px solid transparent;
    border-right: 20px solid transparent;
 }
`

export const NotificationAction = styled<{}, 'a'>('a')`
  color: #fff;
  text-decoration: underline;
`

export const CloseIcon = styled<{}, 'div'>('div')`
  color: #fff;
  width: 13px;
  height: 13px;
  float: right;
  cursor: pointer;
  margin-top: 2px;
`

export const NotificationCloseIcon = styled<{}, 'div'>('div')`
  color: #fff;
  width: 15px;
  height: 15px;
  position: absolute;
  cursor: pointer;
  top: 18px;
  right: 20px;
`

export const UnsupportedMessage = styled<{}, 'div'>('div')`
  color: rgba(255, 255, 255, 0.70);
  font-size: 14px;
  max-width: 235px;
  margin-top: 8px;
`

export const TurnOnText = styled<{}, 'div'>('div')`
  font-size: 14px;
  color: ${palette.grey300};
  margin-top: 8px;
`

export const StyledTOS = styled(TOSAndPP as React.ComponentType<TOSProps>)`
  font-size: 12px;

  a {
    color: ${p => p.theme.color.brandBrave};
  }
`

export const NotificationContent = styled<{}, 'div'>('div')`
  padding: 3px;
  font-family: Poppins, sans-serif;
  font-size: 13px;
  font-weight: 600;
`

export const NotificationTOS = styled<{}, 'div'>('div')`
  font-weight: 500;
  font-size: 11px;
  line-height: 17px;
`
