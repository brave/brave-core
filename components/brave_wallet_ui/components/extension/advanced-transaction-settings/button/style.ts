// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import { SettingsAdvancedIcon } from 'brave-ui/components/icons'

import { WalletButton } from '../../../shared/style'

export const StyledButton = styled(WalletButton)`
  display: flex;
  width: 30px;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  cursor: pointer;
  outline: none;
  padding: 10px 0px 0px 0px;
  border: none;
  background: none;
`

export const SettingsIcon = styled(SettingsAdvancedIcon)`
  padding-bottom: 12px;
  color: ${(p) => p.theme.color.text03};
`

export const TabLine = styled.div`
  display: flex;
  width: 100%;
  height: 2px;
  background: ${(p) => p.theme.color.divider01};
`
