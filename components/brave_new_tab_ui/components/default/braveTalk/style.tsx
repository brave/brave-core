/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import palette from 'brave-ui/theme/colors'

export const WidgetWrapper = styled('div')<{}>`
  color: white;
  padding: 6px 20px 12px 20px;
  border-radius: 6px;
  position: relative;
  font-family: ${p => p.theme.fontFamily.body};
  overflow: hidden;
  min-width: 284px;
  min-height: initial;
  backdrop-filter: blur(23px);
  background: rgba(33, 37, 41, 0.2);
`

export const Header = styled('div')<{}>`
  text-align: left;
`

export const Content = styled('div')<{}>`
  margin-top: 20px;
  margin-bottom: 10px;
  min-width: 200px;
`

export const WelcomeText = styled('span')<{}>`
  display: block;
  font-size: 14px;
  font-weight: 500;
  text-align: center;
  color: ${palette.white};
  max-width: 80%;
  margin: 0 auto;
`

export const ActionsWrapper = styled('div')<{}>`
  text-align: center;
`

export const CallButton = styled('button')<{}>`
  font-size: 14px;
  font-weight: bold;
  border-radius: 20px;
  background: ${palette.blurple500};
  border: 0;
  padding: 10px 30px;
  cursor: pointer;
  color: ${palette.white};
  margin: 20px 0;
  outline: none;

  &:focus {
    outline: none;
    box-shadow: 0 0 0 1px ${p => p.theme.palette.blurple200};
  }
`

export const BraveTalkIcon = styled('div')<{}>`
  width: 24px;
  height: 24px;
  margin-right: 7px;
  margin-left: 5px;
  margin-top: 2px;
`

export const StyledTitle = styled('div')<{}>`
  margin-top: 6px;
  display: flex;
  justify-content: flex-start;
  align-items: center;
  font-size: 18px;
  font-weight: 600;
  color: ${palette.white};
  font-family: ${p => p.theme.fontFamily.heading};
`

export const Privacy = styled('div')<{}>`
  width: 100%;
  text-align: center;
`

export const PrivacyLink = styled('a')<{}>`
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  color: #FFFFFF;
  opacity: 0.8;
  text-decoration: none;
  font-family: ${p => p.theme.fontFamily.heading};
`
