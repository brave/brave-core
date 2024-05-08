// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { Text, WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: column;
  width: 100vw;
  height: 100%;
  background-color: ${leo.color.container.highlight};
`

export const MessageBox = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  width: 100%;
  max-height: 280px;
  padding: 12px 16px;
  background-color: ${leo.color.container.background};
  border-radius: 12px;
  overflow-x: hidden;
  overflow-y: auto;
`

export const Title = styled(Text)`
  line-height: 26px;
  color: ${leo.color.text.primary};
`

export const Description = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.secondary};
`

export const OriginName = styled(Title)`
  line-height: 28px;
`

export const MessageText = styled(Title)`
  line-height: 24px;
  text-align: left;
  flex-wrap: wrap;
`

export const URLText = styled(MessageText)`
  word-break: break-all;
`

export const DetailsKeyText = styled(Title)`
  color: ${leo.color.text.secondary};
  line-height: 24px;
  text-align: left;
  flex-wrap: wrap;
  min-width: 25%;
  max-width: 25%;
  margin-right: 20px;
`

export const CodeBlock = styled.code``

export const DetailsInfoText = styled(Title)`
  color: ${leo.color.text.primary};
  line-height: 18px;
  text-align: left;
  flex-wrap: wrap;
  word-wrap: break-word;
  max-width: 65%;
`

export const FavIcon = styled.img`
  width: 40px;
  height: 40px;
  border-radius: 5px;
  margin-bottom: 8px;
`

export const IconButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
  padding: 0px;
  margin: 0px;
  background-color: transparent;
`

export const CloseIcon = styled(Icon).attrs({
  name: 'close'
})`
  --leo-icon-size: 24px;
  color: ${leo.color.icon.default};
`

export const ErrorBox = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 100%;
  padding: 16px 25px;
  background-color: ${leo.color.systemfeedback.errorBackground};
  border-radius: 12px;
  margin-bottom: 16px;
  overflow-wrap: anywhere;
`

export const ErrorDescriptionText = styled(Text)`
  display: inline-block;
  line-height: 24px;
  color: ${leo.color.text.primary};
  text-align: center;
`

export const WarningIcon = styled(Icon).attrs({
  name: 'warning-circle-filled'
})`
  --leo-icon-size: 32px;
  margin-bottom: 8px;
  color: ${leo.color.systemfeedback.errorIcon};
`

export const OriginErrorText = styled(Text)`
  line-height: 18px;
  color: ${leo.color.systemfeedback.errorIcon};
  margin-bottom: 16px;
`

export const LaunchButton = styled(IconButton)`
  display: inline;
  margin-left: 4px;
  bottom: -3px;
  position: relative;
`

export const LaunchIcon = styled(Icon).attrs({
  name: 'launch'
})`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.interactive};
`
