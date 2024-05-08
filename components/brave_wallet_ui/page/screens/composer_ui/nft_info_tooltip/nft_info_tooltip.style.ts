// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'

// Shared Styles
import { WalletButton } from '../../../../components/shared/style'

export const Wrapper = styled.div`
  display: flex;
  position: relative;
  height: 100%;
  align-items: center;
  width: 40px;
  justify-content: flex-end;
`

export const Tip = styled.div`
  position: absolute;
  border-radius: 16px;
  padding: 16px;
  z-index: 10;
  right: 8px;
  top: 42px;
  width: 220px;
  background-color: ${leo.color.container.background};
  border: 1px solid ${leo.color.divider.subtle};
  box-shadow: 0px 4px 20px rgba(0, 0, 0, 0.1);
  white-space: normal;
`

export const TipIcon = styled(Icon).attrs({
  name: 'info-outline'
})`
  --leo-icon-size: 14px;
  color: ${leo.color.icon.interactive};
  margin-right: 8px;
`

export const AddressLink = styled(WalletButton)`
  background-color: none;
  background: none;
  border: none;
  outline: none;
  cursor: pointer;
  color: ${leo.color.text.interactive};
  font-size: 14px;
  padding: 0px;
  font-family: 'Poppins';
`
