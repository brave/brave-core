/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'
import { EmoteSadIcon } from 'brave-ui/components/icons'

export const StyledImage = styled.img`
  height: 79px;
  width: 80px;
  display: block;
  margin-top: 15px;
  margin-left: auto;
  margin-right: auto;
`

export const StyledNoticeLink = styled('a')<{}>`
  color: #0095FF;
  font-size: 12px;
  text-decoration: none;
`

export const ConnectWallet = styled.div`
  font-size: 12px;
  color: ${p => p.theme.palette.grey600};
  margin-top: 20px;
  text-align: center;
`

export const UnverifiedWalletMessage = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  font-size: 16px;
  color: ${p => p.theme.palette.grey600};
  margin: 30px 30px 120px 30px;
  opacity: 1;
  text-align: center;
`

export const LineSeparator = styled.div`
  border-bottom: solid 1px ${p => p.theme.color.separatorLine};
  text-align: center;
`

export const UnverifiedWalletTitle = styled.div`
  font-family: ${p => p.theme.fontFamily.body};
  color: ${p => p.theme.palette.grey800};
  font-size: 22px;
  font-weight: bold;
  margin-top: 25px;
  text-align: center;
`

export const DefaultEmoteSadIcon = styled(EmoteSadIcon)`
  height: 22px;
  width: 22px;
  position: relative;
  margin-right: 10px;
  vertical-align: middle;
`
