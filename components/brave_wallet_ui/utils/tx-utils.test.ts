// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getTransactionStatusString } from './tx-utils'

describe('Check Transaction Status Strings Value', () => {
  test('Transaction ID 0 should return Unapproved', () => {
    expect(getTransactionStatusString(0)).toEqual('braveWalletTransactionStatusUnapproved')
  })
  test('Transaction ID 1 should return Approved', () => {
    expect(getTransactionStatusString(1)).toEqual('braveWalletTransactionStatusApproved')
  })

  test('Transaction ID 2 should return Rejected', () => {
    expect(getTransactionStatusString(2)).toEqual('braveWalletTransactionStatusRejected')
  })

  test('Transaction ID 3 should return Submitted', () => {
    expect(getTransactionStatusString(3)).toEqual('braveWalletTransactionStatusSubmitted')
  })

  test('Transaction ID 4 should return Confirmed', () => {
    expect(getTransactionStatusString(4)).toEqual('braveWalletTransactionStatusConfirmed')
  })

  test('Transaction ID 5 should return Error', () => {
    expect(getTransactionStatusString(5)).toEqual('braveWalletTransactionStatusError')
  })

  test('Transaction ID 6 should return Dropped', () => {
    expect(getTransactionStatusString(6)).toEqual('braveWalletTransactionStatusDropped')
  })

  test('Transaction ID 7 should return Signed', () => {
    expect(getTransactionStatusString(7))
      .toEqual('braveWalletTransactionStatusSigned')
  })

  test('Transaction ID 8 should return an empty string', () => {
    expect(getTransactionStatusString(8)).toEqual('')
  })
})
