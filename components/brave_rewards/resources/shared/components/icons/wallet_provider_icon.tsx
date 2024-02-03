/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ExternalWalletProvider } from '../../lib/external_wallet'
import { BitflyerIcon } from './bitflyer_icon'
import { GeminiIcon } from './gemini_icon'
import { UpholdIcon } from './uphold_icon'
import { ZebPayIcon } from './zebpay_icon'
import { SolanaIcon } from './solana_icon'

interface Props {
  provider: ExternalWalletProvider
}

export function WalletProviderIcon (props: Props) {
  switch (props.provider) {
    case 'gemini': return <GeminiIcon />
    case 'bitflyer': return <BitflyerIcon />
    case 'uphold': return <UpholdIcon />
    case 'zebpay': return <ZebPayIcon />
    case 'solana': return <SolanaIcon />
  }
}
