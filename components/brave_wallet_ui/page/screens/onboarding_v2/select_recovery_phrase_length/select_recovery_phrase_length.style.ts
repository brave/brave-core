// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as nala from '@brave/nala/tokens/css/variables'
import Icon from '@brave/nala/react/icon'
import NalaLabel from '@brave/nala/react/label'

// Shared Styles
import { WalletButton } from '$wallet/components/shared/style'

export const OptionButton = styled(WalletButton)<{
  isSelected: boolean
}>`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  background: none;
  cursor: pointer;
  outline: none;
  margin: 0px;
  background-color: ${({ isSelected }) =>
    isSelected
      ? `color-mix(in srgb, ${nala.color.primitive.brands.rorange2} 4%, ${nala.color.container.background})`
      : nala.color.container.background};
  border-radius: ${nala.radius.l};
  border: 2px solid
    ${({ isSelected }) =>
      isSelected
        ? nala.color.primitive.brands.rorange2
        : nala.color.divider.subtle};
  padding: ${nala.spacing.xl};
  width: 100%;
  gap: 16px;
`

export const RadioIcon = styled(Icon)<{
  isSelected: boolean
}>`
  --leo-icon-size: 24px;
  --leo-icon-color: ${({ isSelected }) =>
    isSelected
      ? nala.color.primitive.brands.rorange2
      : nala.color.icon.default};
`

export const Label = styled(NalaLabel)`
  /* The label paints its text with an inline color inside the shadow root,
     so a host color property never reaches it. Override the token it reads. */
  --leo-color-neutral-60: ${nala.color.primitive.brands.rorange2};
`
