// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { lamportsToSol } from './web3-utils'

describe('lamportsToSol', () => {
  it('should convert a lamport-denominated value to a SOL denominated value', () => {
    expect(lamportsToSol('1').format()).toEqual('0.000000001')
    expect(lamportsToSol('10').format()).toEqual('0.00000001')
    expect(lamportsToSol('100').format()).toEqual('0.0000001')
    expect(lamportsToSol('1000').format()).toEqual('0.000001')
    expect(lamportsToSol('10000').format()).toEqual('0.00001')
    expect(lamportsToSol('100000').format()).toEqual('0.0001')
    expect(lamportsToSol('1000000').format()).toEqual('0.001')
    expect(lamportsToSol('10000000').format()).toEqual('0.01')
    expect(lamportsToSol('100000000').format()).toEqual('0.1')
    expect(lamportsToSol('1000000000').format()).toEqual('1')
    expect(lamportsToSol('10000000000').format()).toEqual('10')
  })
})
