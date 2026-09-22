// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as leo from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'

// Assets
import WalletLogoLight from '$wallet/assets/svg-icons/wallet_logo_light.svg'
import WalletLogoDark from '$wallet/assets/svg-icons/wallet_logo_dark.svg'

// Shared Styles
import { Column } from '$wallet/components/shared/style'

export const Wrapper = styled(Column)`
  background-color: ${leo.color.container.background};
`

export const WalletLogo = styled.div`
  height: 32px;
  width: 100px;
  background-image: url(${WalletLogoLight});
  background-size: cover;
  @media (prefers-color-scheme: dark) {
    background-image: url(${WalletLogoDark});
  }
`

export const CardStack = styled.div`
  --card-peek: 70px;
  --card-hover-lift: 14px;

  display: flex;
  flex-direction: column;
  width: 100%;
  perspective: 900px;
  perspective-origin: 50% 30%;
  /* Allow tilted cards to project out of the stack. */
  overflow: visible;
  padding-top: var(--card-hover-lift);
  margin-top: calc(-1 * var(--card-hover-lift));

  & > * + * {
    margin-top: calc(var(--card-peek) - var(--card-height, 220px));
  }

  & > *:hover {
    --card-lift: calc(-1 * var(--card-hover-lift));
  }
`
