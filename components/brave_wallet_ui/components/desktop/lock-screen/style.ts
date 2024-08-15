// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import Icon from '@brave/leo/react/icon'
import * as leo from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'

// Assets
import {
  UnlockIconDark,
  UnlockIconLight
} from '../../../assets/svg-icons/unlock-wallet-icons'

// Shared Styles
import { Text, Column, WalletButton } from '../../shared/style'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: 50px 0px;
`

export const Title = styled(Text)`
  font-size: 28px;
  font-weight: 500;
  line-height: 40px;
  color: ${leo.color.text.primary};
  margin-bottom: 8px;
  text-align: center;
`

export const Description = styled(Text)`
  font-weight: 400;
  line-height: 28px;
  color: ${leo.color.text.secondary};
  margin-bottom: 40px;
  text-align: center;
`

export const InputLabel = styled(Text)`
  line-height: 18px;
  color: ${leo.color.text.primary};
`

export const PageIcon = styled.div`
  width: 92px;
  height: 100px;
  background: url(${UnlockIconLight});
  background-repeat: no-repeat;
  background-size: 100%;
  margin-bottom: 24px;
  @media (prefers-color-scheme: dark) {
    background: url(${UnlockIconDark});
  }
`

export const InputColumn = styled(Column)`
  max-width: 400px;
`

export const UnlockButton = styled(Button)`
  width: 100%;
  margin-bottom: 12px;
`

export const AndroidLockScreenWrapper = styled(WalletButton)`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background: none;
  background-color: none;
  border: none;
  outline: none;
  margin: 0px;
  padding: 12px;
  top: 0px;
  right: 0px;
  bottom: 0px;
  left: 0px;
  gap: 8px;
  z-index: 100;
  position: absolute;
`

export const DoubleTapIcon = styled(Icon).attrs({
  name: 'finger-double-tap'
})`
  --leo-icon-size: 72px;
  color: ${leo.color.icon.interactive};
  margin-bottom: 16px;
`
