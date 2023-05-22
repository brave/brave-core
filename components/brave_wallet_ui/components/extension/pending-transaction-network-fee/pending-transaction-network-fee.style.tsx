// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import { SettingsAdvancedIcon } from 'brave-ui/components/icons'

import { WalletButton } from '../../shared/style'

export const NetworkFeeAndSettingsContainer = styled.div`
  display: flex;
  justify-content: space-between;
  width: calc(100% - 8px);
`

export const NetworkFeeContainer = styled.div``

export const NetworkFeeTitle = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 12px;
  line-height: 20px;
  color: ${p => p.theme.color.text03};
`

export const NetworkFeeValue = styled.div`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 18px;
  display: flex;
  align-items: center;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text02};
  gap: 6px;
`

export const Settings = styled(WalletButton)`
  display: flex;
  align-self: flex-start;
  cursor: pointer;
  outline: none;
  border: none;
  background: none;
`

export const SettingsIcon = styled(SettingsAdvancedIcon)`
  width: 14px;
  color: ${p => p.theme.color.text03};
`
