// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/nala/tokens/css/variables'
import Icon from '@brave/nala/react/icon'

// Shared Styles
import { WalletButton } from '$wallet/components/shared/style'

export const WelcomeActionWrapper = styled(WalletButton)`
  display: flex;
  flex-direction: column;
  justify-content: flex-start;
  align-items: flex-start;
  padding: ${leo.spacing.xl};
  gap: ${leo.spacing.l};
  width: 100%;
  background: ${leo.color.container.background};
  border-radius: ${leo.radius.xl};
  box-shadow: 0px 4px 16px -2px rgba(0, 0, 0, 0.08);
  border: 1px solid transparent;
  cursor: pointer;
  transition:
    background-color 150ms ease,
    border-color 150ms ease;
  &:hover,
  &:focus-visible {
    background: color-mix(
      in srgb,
      ${leo.color.primitive.brands.rorange2} 4%,
      ${leo.color.container.background}
    );
    border-color: ${leo.color.primitive.brands.rorange2};
    outline: none;
  }
`

export const IconWrapper = styled.div`
  --leo-icon-size: 18px;
  --leo-icon-color: ${leo.color.primitive.brands.rorange2};
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 42px;
  min-height: 42px;
  background-color: color-mix(
    in srgb,
    ${leo.color.primitive.brands.rorange2} 10%,
    transparent
  );
  border-radius: ${leo.radius.full};
`

export const WalletIcon = styled(Icon)`
  --leo-icon-size: 20px;
  border-radius: 50%;
`
