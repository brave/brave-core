// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  TransactionInfo
} from '../../constants/types'

export const getMockedTransactionInfo = (): TransactionInfo => {
  return {
    id: '1',
    fromAddress: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
    txHash: '',
    txData: {
      baseData: {
        to: '0x8b52c24d6e2600bdb8dbb6e8da849ed38ab7e81f',
        value: '0x01706a99bf354000',
        data: new Uint8Array(0),
        nonce: '0x03',
        gasLimit: '0x5208',
        gasPrice: '0x22ecb25c00'
      },
      chainId: '1337',
      maxPriorityFeePerGas: '',
      maxFeePerGas: ''
    },
    txStatus: 1,
    txType: 5,
    txParams: [],
    txArgs: [],
    createdTime: { microseconds: 0 },
    submittedTime: { microseconds: 0 },
    confirmedTime: { microseconds: 0 }
  }
}
