// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mockSolDappSignAndSendTransactionRequest } from '../common/constants/mocks'

import {
  getSolanaTransactionInstructionParamsAndType
} from './solana-instruction-utils'

describe('getSolanaSystemInstructionParamsAndType', () => {
  it('converts a brave wallet instruction to a solana instruction', () => {
    const typedInstruction = getSolanaTransactionInstructionParamsAndType(
      mockSolDappSignAndSendTransactionRequest.txDataUnion.solanaTxData!
        .instructions[0]
    )

    const paramNames = typedInstruction.params.map((p) => p.name)
    const hasLamportsParam = paramNames.includes('lamports')
    const accountParamNames = typedInstruction.accountParams.map((p) => p.name)
    const hasFromPubkeyParam = accountParamNames.includes('from_account')
    const hasToPubkeyParam = accountParamNames.includes('to_account')
    const fromAccountParamIndex = accountParamNames.indexOf('from_account')
    const toAccountParamIndex = accountParamNames.indexOf('to_account')
    const hasFromAddrTableLookupIndex =
      mockSolDappSignAndSendTransactionRequest
        .txDataUnion.solanaTxData
        ?.instructions[0]
        .accountMetas[fromAccountParamIndex]
        .addrTableLookupIndex?.val !== undefined
    const hasToAddrTableLookupIndex =
      mockSolDappSignAndSendTransactionRequest
        .txDataUnion.solanaTxData
        ?.instructions[0]
        .accountMetas[toAccountParamIndex]
        .addrTableLookupIndex
        ?.val !== undefined

    expect(typedInstruction.params).toBeDefined()
    expect(hasLamportsParam).toEqual(true)
    expect(hasFromPubkeyParam).toEqual(true)
    expect(hasToPubkeyParam).toEqual(true)
    expect(hasFromAddrTableLookupIndex).toEqual(false)
    expect(hasToAddrTableLookupIndex).toEqual(true)
  })
})
