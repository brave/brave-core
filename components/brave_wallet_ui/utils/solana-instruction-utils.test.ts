// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as Solana from '@solana/web3.js'
import { mockSolDappSignAndSendTransactionRequest } from '../common/constants/mocks'

import {
  getSolanaInstructionParamKeyName,
  getSolanaTransactionInstructionParamsAndType
} from './solana-instruction-utils'

describe('getSolanaSystemInstructionParamsAndType', () => {
  it('converts a brave wallet instruction to a solana instruction', () => {
    const typedInstruction = getSolanaTransactionInstructionParamsAndType(
      mockSolDappSignAndSendTransactionRequest.txDataUnion.solanaTxData.instructions[0]
    )

    expect(typedInstruction.instruction).toBeInstanceOf(Solana.TransactionInstruction)
    expect(typedInstruction.params).toBeDefined()
    expect(typedInstruction.params).toHaveProperty('lamports')
  })
})

describe('getSolanaInstructionParamKeyName', () => {
  it('returns the key name if translation/mapping not found', () => {
    expect(getSolanaInstructionParamKeyName('madeUpKey' as any)).toBe('madeUpKey')
  })
})
