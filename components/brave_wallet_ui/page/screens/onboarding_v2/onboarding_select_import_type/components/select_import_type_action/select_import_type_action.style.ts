// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import Icon from '@brave/nala/react/icon'
import * as nala from '@brave/nala/tokens/css/variables'

// Shared Styles
import { Text } from '$wallet/components/shared/style'

export const ActionWrapper = styled.button`
  display: flex;
  flex-direction: column;
  justify-content: start;
  width: 100%;
  padding: 24px;
  border-radius: 8px;
  background-color: transparent;
  cursor: pointer;
  border: none;
  outline: none;
  gap: 10px;
`

export const ArrowIconWrapper = styled.div`
  --leo-icon-size: 18px;
  --leo-icon-color: ${nala.color.primitive.brands.rorange2};
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 28px;
  min-height: 28px;
  background-color: color-mix(
    in srgb,
    ${nala.color.primitive.brands.rorange2} 10%,
    transparent
  );
  border-radius: ${nala.radius.full};
`

export const ActionText = styled(Text)`
  width: 100%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
`

export const ActionIcon = styled(Icon)`
  --leo-icon-size: 24px;
  --leo-icon-color: ${nala.color.icon.default};
  display: flex;
  align-items: center;
  justify-content: center;
`

export const IconsWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  flex-wrap: wrap;
  gap: 12px;
`
