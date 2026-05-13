// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../constants/types'

// Utils
import {
  hasPermitShape,
  isPermit2Domain,
  isPermitLikeEthSignTypedData,
  PERMIT2_ADDRESS,
  PERMIT_LIKE,
} from './sign_utils'

function makeEthSignTypedData(
  overrides: Partial<BraveWallet.EthSignTypedData>,
): BraveWallet.EthSignTypedData {
  return {
    addressParam: '0x',
    chainId: '1',
    domainHash: [],
    domainJson: '{}',
    messageJson: '{}',
    meta: undefined,
    primaryHash: [],
    primaryType: 'Person',
    typesJson: '{}',
    ...overrides,
  }
}

function typesJsonForPrimary(
  primaryType: string,
  fields: { name: string; type: string }[],
): string {
  return JSON.stringify({ [primaryType]: fields })
}

describe('hasPermitShape', () => {
  it('returns false when typesJson is empty', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Foo',
          typesJson: '',
        }),
      ),
    ).toBe(false)
  })

  it('returns false when typesJson is invalid JSON', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Foo',
          typesJson: '{',
        }),
      ),
    ).toBe(false)
  })

  it('returns true when permit shape is in a non-primary type definition', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Nested',
          typesJson: JSON.stringify({
            Nested: [{ name: 'payload', type: 'Other' }],
            Other: [
              { name: 'spender', type: 'address' },
              { name: 'value', type: 'uint256' },
            ],
          }),
        }),
      ),
    ).toBe(true)
  })

  it('returns true when permit2 authorization is nested under a benign primary type', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Nested',
          typesJson: JSON.stringify({
            Nested: [{ name: 'permit', type: 'PermitSingle' }],
            PermitSingle: [
              { name: 'details', type: 'PermitDetails' },
              { name: 'spender', type: 'address' },
              { name: 'sigDeadline', type: 'uint256' },
            ],
          }),
        }),
      ),
    ).toBe(true)
  })

  it('returns false when spender is missing', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Foo',
          typesJson: typesJsonForPrimary('Foo', [
            { name: 'owner', type: 'address' },
            { name: 'value', type: 'uint256' },
          ]),
        }),
      ),
    ).toBe(false)
  })

  it('returns false when spender exists but no value, amount, or permit nested types', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Foo',
          typesJson: typesJsonForPrimary('Foo', [
            { name: 'spender', type: 'address' },
            { name: 'owner', type: 'address' },
          ]),
        }),
      ),
    ).toBe(false)
  })

  it('returns true for spender and value', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'CustomPermit',
          typesJson: typesJsonForPrimary('CustomPermit', [
            { name: 'owner', type: 'address' },
            { name: 'spender', type: 'address' },
            { name: 'value', type: 'uint256' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for spender and amount', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'CustomPermit',
          typesJson: typesJsonForPrimary('CustomPermit', [
            { name: 'spender', type: 'address' },
            { name: 'amount', type: 'uint256' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for spender and PermitDetails field', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'PermitSingle',
          typesJson: typesJsonForPrimary('PermitSingle', [
            { name: 'details', type: 'PermitDetails' },
            { name: 'spender', type: 'address' },
            { name: 'sigDeadline', type: 'uint256' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for spender and PermitDetails[] field', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'PermitBatch',
          typesJson: typesJsonForPrimary('PermitBatch', [
            { name: 'details', type: 'PermitDetails[]' },
            { name: 'spender', type: 'address' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for spender and TokenPermissions field', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'PermitTransferFrom',
          typesJson: typesJsonForPrimary('PermitTransferFrom', [
            { name: 'permitted', type: 'TokenPermissions' },
            { name: 'spender', type: 'address' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for spender and TokenPermissions[] field', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'PermitBatchTransferFrom',
          typesJson: typesJsonForPrimary('PermitBatchTransferFrom', [
            { name: 'permitted', type: 'TokenPermissions[]' },
            { name: 'spender', type: 'address' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for DAI-style spender and allowed(bool)', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Permit',
          typesJson: typesJsonForPrimary('Permit', [
            { name: 'holder', type: 'address' },
            { name: 'spender', type: 'address' },
            { name: 'nonce', type: 'uint256' },
            { name: 'expiry', type: 'uint256' },
            { name: 'allowed', type: 'bool' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns false when allowed is present but not bool', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'Permit',
          typesJson: typesJsonForPrimary('Permit', [
            { name: 'spender', type: 'address' },
            { name: 'allowed', type: 'uint256' },
          ]),
        }),
      ),
    ).toBe(false)
  })

  it('returns true for EIP-3009 authorization field set', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'TransferWithAuthorization',
          typesJson: typesJsonForPrimary('TransferWithAuthorization', [
            { name: 'from', type: 'address' },
            { name: 'to', type: 'address' },
            { name: 'value', type: 'uint256' },
            { name: 'validAfter', type: 'uint256' },
            { name: 'validBefore', type: 'uint256' },
            { name: 'nonce', type: 'bytes32' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns false when EIP-3009 authorization fields are incomplete', () => {
    expect(
      hasPermitShape(
        makeEthSignTypedData({
          primaryType: 'TransferWithAuthorization',
          typesJson: typesJsonForPrimary('TransferWithAuthorization', [
            { name: 'from', type: 'address' },
            { name: 'to', type: 'address' },
            { name: 'value', type: 'uint256' },
            { name: 'nonce', type: 'bytes32' },
          ]),
        }),
      ),
    ).toBe(false)
  })
})

describe('isPermit2Domain', () => {
  it('returns true when verifyingContract is the Permit2 contract', () => {
    expect(
      isPermit2Domain(
        makeEthSignTypedData({
          domainJson: JSON.stringify({
            name: 'Permit2',
            chainId: '1',
            verifyingContract: PERMIT2_ADDRESS,
          }),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for checksummed Permit2 address', () => {
    expect(
      isPermit2Domain(
        makeEthSignTypedData({
          domainJson: JSON.stringify({
            verifyingContract: '0x000000000022D473030F116DDeE9F6b43ac78BA3',
          }),
        }),
      ),
    ).toBe(true)
  })

  it('returns false for other verifyingContract addresses', () => {
    expect(
      isPermit2Domain(
        makeEthSignTypedData({
          domainJson: JSON.stringify({
            verifyingContract: '0x0000000000000000000000000000000000000001',
          }),
        }),
      ),
    ).toBe(false)
  })

  it('returns false when domainJson is invalid', () => {
    expect(
      isPermit2Domain(
        makeEthSignTypedData({
          domainJson: '{',
        }),
      ),
    ).toBe(false)
  })
})

describe('isPermitLikeEthSignTypedData', () => {
  it('returns false for undefined', () => {
    expect(isPermitLikeEthSignTypedData(undefined)).toBe(false)
  })

  it.each(PERMIT_LIKE)('returns true when primaryType is %s', (primaryType) => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType,
          typesJson: '{}',
        }),
      ),
    ).toBe(true)
  })

  it('returns false for non-permit primary type and non-permit shape', () => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType: 'Person',
          typesJson: typesJsonForPrimary('Person', [
            { name: 'name', type: 'string' },
            { name: 'wallet', type: 'address' },
          ]),
        }),
      ),
    ).toBe(false)
  })

  it('returns true for permit-shaped message with unknown primary type name', () => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType: 'VendorCustomPermit',
          typesJson: typesJsonForPrimary('VendorCustomPermit', [
            { name: 'owner', type: 'address' },
            { name: 'spender', type: 'address' },
            { name: 'value', type: 'uint256' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns true for EIP-3009 shape with unknown primary type name', () => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType: 'VendorTransferAuth',
          typesJson: typesJsonForPrimary('VendorTransferAuth', [
            { name: 'from', type: 'address' },
            { name: 'to', type: 'address' },
            { name: 'value', type: 'uint256' },
            { name: 'validAfter', type: 'uint256' },
            { name: 'validBefore', type: 'uint256' },
            { name: 'nonce', type: 'bytes32' },
          ]),
        }),
      ),
    ).toBe(true)
  })

  it('returns false when primaryType is empty and shape does not match', () => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType: '',
          typesJson: '{}',
        }),
      ),
    ).toBe(false)
  })

  it('returns true for nested permit2 type with benign primary type name', () => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType: 'Nested',
          typesJson: JSON.stringify({
            Nested: [{ name: 'permit', type: 'PermitSingle' }],
            PermitSingle: [
              { name: 'details', type: 'PermitDetails' },
              { name: 'spender', type: 'address' },
              { name: 'sigDeadline', type: 'uint256' },
            ],
          }),
        }),
      ),
    ).toBe(true)
  })

  it('returns true when domain verifyingContract is Permit2', () => {
    expect(
      isPermitLikeEthSignTypedData(
        makeEthSignTypedData({
          primaryType: 'Person',
          typesJson: typesJsonForPrimary('Person', [
            { name: 'name', type: 'string' },
          ]),
          domainJson: JSON.stringify({
            verifyingContract: PERMIT2_ADDRESS,
          }),
        }),
      ),
    ).toBe(true)
  })
})
