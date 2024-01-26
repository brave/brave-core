// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import Dropdown from '@brave/leo/react/dropdown'
import * as leo from '@brave/leo/tokens/css'

export const LockIconContainer = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  width: 30px;
  height: 30px;
  background-color: ${leo.color.purple[10]};
  border-radius: 50%;
`

export const LockIcon = styled(Icon).attrs({
  name: 'lock'
})`
  --leo-icon-size: ${leo.icon.xs};
  color: ${leo.color.purple[30]};
`

export const SettingDescription = styled.div`
  font-family: Poppins;
  font-size: 14px;
  font-style: normal;
  font-weight: 400;
  line-height: 22px;
  wrap: nowrap;
`

export const DurationDropdown = styled(Dropdown)`
  min-width: 136px;

`