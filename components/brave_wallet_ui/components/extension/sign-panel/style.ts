// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import { WarningBoxTitleRow } from '../shared-panel-styles'

// Shared Styles
import { Text } from '../../shared/style'

interface StyleProps {
  orb: string
}

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
  background-color: ${leo.color.page.background};
  padding: 0px 16px;
`

export const TopRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-direction: row;
  width: 100%;
  padding: 15px 0px;
`

export const AccountCircle = styled.div<Partial<StyleProps>>`
  width: 54px;
  min-height: 54px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
  margin-bottom: 13px;
`

export const AccountNameText = styled(Text)`
  margin-bottom: 2px;
  max-width: 80%;
  word-break: break-word;
  text-align: center;
`

export const PanelTitle = styled(Text)`
  text-align: center;
  margin-bottom: 15px;
`

export const MessageBox = styled.div<{ height?: string; width?: string }>`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: column;
  border: 1px solid ${leo.color.divider.subtle};
  box-sizing: border-box;
  border-radius: 4px;
  width: ${(p) => (p.width ? p.width : '255px')};
  height: ${(p) => (p.height ? p.height : '140px')};
  padding: 8px 14px;
  margin-bottom: 14px;
  overflow-x: hidden;
  overflow-y: scroll;
`

export const MessageHeader = styled(Text)`
  text-align: left;
  word-break: break-word;
  white-space: pre-wrap;
`

export const MessageHeaderSection = styled(MessageHeader)`
  margin: auto;
  text-align: center;
`

export const MessageText = styled(Text)`
  text-align: left;
  word-break: break-word;
  white-space: pre-wrap;
`

export const SignPanelButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 100%;
  margin-bottom: 14px;
  gap: 8px;
`

export const WarningTitleRow = styled(WarningBoxTitleRow)`
  margin-bottom: 8px;
`

export const HeaderTitle = styled.div`
  font: ${leo.font.large.semibold};
  display: flex;
  align-items: center;
  text-align: center;
  color: ${leo.color.text.primary};
  margin: 4px 0 8px 0;
`
