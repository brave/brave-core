/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

interface StyleProps {
  isLast?: boolean
}

export const WidgetWrapper = styled<{}, 'div'>('div')`
  color: white;
  padding: 10px 15px;
  border-radius: 6px;
  position: relative;
  font-family: Muli, sans-serif;
  overflow-x: hidden;
  background-image: linear-gradient(140deg, #392DD1 0%, #8E2995 100%);
`

export const Footer = styled<{}, 'div'>('div')`
  max-width: 275px;
  margin-top: 25px;
`

export const BatIcon = styled<{}, 'div'>('div')`
  width: 30px;
  height: 30px;
  display: inline-block;
`

export const RewardsTitle = styled<{}, 'span'>('span')`
  top: -9px;
  left: 8px;
  font-size: 14px;
  font-weight: 500;
  position: relative;
  font-family: Poppins, sans-serif;
`

export const ServiceText = styled<{}, 'span'>('span')`
  color: #fff;
  font-size: 10px;
  letter-spacing: 0;
  line-height: 18px;
`

export const ServiceLink = styled<{}, 'a'>('a')`
  color: #B0DBFF;
  font-weight: 600;
  text-decoration: none;
`

export const LearnMoreLink = styled(ServiceLink)`
  margin-right: 5px;
`

export const LearnMoreText = styled<{}, 'div'>('div')`
  font-size: 14px;
  margin-top: 50px;
`

export const PreOptInInfo = styled<{}, 'div'>('div')`
  margin-top: 20px;
`

export const Title = styled<{}, 'span'>('span')`
  font-size: 19px;
  display: block;
  font-family: Poppins, sans-serif;
`

export const SubTitle = styled<{}, 'span'>('span')`
  font-size: 14px;
  display: block;
  margin-top: 10px;
  max-width: 250px;
`

export const PreOptInAction = styled<{}, 'div'>('div')`
  margin-top: 30px;
  text-align: center
`

export const TurnOnButton = styled<{}, 'button'>('button')`
  border-radius: 20px;
  background: white;
  color: ${palette.blurple500};
  font-weight: bold;
  font-size: 14px;
  padding: 7px 60px;
  margin: 0 auto;
  cursor: pointer;
  border: none;
`

export const TurnOnAdsButton = styled(TurnOnButton)`
  width: 100%;
  margin-top: 8px;
  display: block;
`

export const AmountItem = styled<StyleProps, 'div'>('div')`
  margin-top: ${p => p.isLast ? 20 : 10}px;
  margin-bottom: ${p => p.isLast ? -10 : 0}px;
`

export const AmountInformation = styled<{}, 'div'>('div')`
  margin-top: 5px;
`

export const Amount = styled<{}, 'span'>('span')`
  font-size: 32px;
  margin-right: 10px;
  font-family: Poppins, sans-serif;
`

export const ConvertedAmount = styled<{}, 'span'>('span')`
  font-size: 14px;
`

export const AmountDescription = styled<{}, 'span'>('span')`
  font-size: 14px;
  color: #fff;
  margin-bottom: 5px;
`

export const NotificationWrapper = styled(WidgetWrapper)`
  position: absolute;
  bottom: 0;
  left: 0;
  width: 100%;
  min-height: 210px;
  box-shadow: 5px 10px 8px 10px #000;
`

export const NotificationButton = styled(TurnOnButton)`
  background: #1DB100;
  color: #fff;
  border: none;
  font-weight: bold;
  padding-top: 10px;
  padding-bottom: 10px;
  margin-top: 50px;
  font-size: 13px;
`

export const Content = styled<{}, 'div'>('div')`
  margin-top: 30px;
`

export const CloseIcon = styled<{}, 'div'>('div')`
  color: #fff;
  width: 20px;
  height: 20px;
  float: right;
  cursor: pointer;
`

export const UnsupportedMessage = styled<{}, 'div'>('div')`
  color: rgba(255, 255, 255, 0.70);
  font-size: 14px;
  max-width: 235px;
  margin-top: 8px;
`
