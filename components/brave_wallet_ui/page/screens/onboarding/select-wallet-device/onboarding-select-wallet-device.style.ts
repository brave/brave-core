// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'

import TrezorLight from './images/trezor-light.svg'
import TrezorDark from './images/trezor-dark.svg'
import LedgerLight from './images/ledger-light.svg'
import LedgerDark from './images/ledger-dark.svg'

import { HardwareVendor } from '../../../../common/api/hardware_keyrings'

export const HardwareWalletGraphic = styled.div<{
  hardwareVendor: HardwareVendor
}>`
  display: flex;
  flex-direction: column;
  justify-content: flex-end;
  width: 321px;
  height: 179px;

  background-image: ${(p) =>
    `url(${p.hardwareVendor === 'Trezor' ? TrezorLight : LedgerLight})`};

  @media (prefers-color-scheme: dark) {
    background-image: ${(p) =>
      `url(${p.hardwareVendor === 'Trezor' ? TrezorDark : LedgerDark})`};
  }
  background-color: ${leo.color.container.background};
  background-size: contain;
  background-repeat: no-repeat;
  position: relative;
`
