// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { AssetIconProps, AssetIconFactory, Column } from '../../shared/style'

export const AssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})

export const AssetNameText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 16px;
  line-height: 18px;
  font-weight: 600;
  color: ${leo.color.text.primary};
`

export const NetworkDescriptionText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  line-height: 18px;
  font-weight: 400;
  color: ${leo.color.text.secondary};
`

export const PriceText = styled.span`
  font-family: Poppins;
  font-style: normal;
  font-size: 14px;
  line-height: 24px;
  font-weight: 600;
  color: ${leo.color.text.primary};
`

export const PercentChange = styled.div<{ isDown?: boolean }>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  font-family: Poppins;
  font-style: normal;
  font-size: 12px;
  font-weight: 400;
  line-height: 18px;
  color: ${(p) =>
    p.isDown
      ? leo.color.systemfeedback.errorIcon
      : leo.color.systemfeedback.successIcon};
`

export const UpDownIcon = styled(Icon)<{
  size?: number
}>`
  --leo-icon-size: 24px;
  color: inherit;
`

export const IconsWrapper = styled(Column)`
  position: relative;
`

export const NetworkIconWrapper = styled(Column)`
  position: absolute;
  bottom: 0px;
  right: 0px;
  background-color: ${leo.color.container.background};
  border-radius: 100%;
  padding: 2px;
`
