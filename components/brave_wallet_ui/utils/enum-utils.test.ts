// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getGetCleanedMojoEnumKeys } from "./enum-utils";

/**
 * @enum {number}
 */
export const MockedJsonRpcErrorEnum = {
  kParsingError: -32700,
  kInvalidRequest: -32600,
  kMethodNotFound: -32601,
  kInvalidParams: -32602,
  kInternalError: -32603,
  MIN_VALUE: -32700,
  MAX_VALUE: -32600,
};

describe('getGetCleanedMojoEnumKeys', () => {
  it('should return a list of the keys from an enum, with any leading "k"s removed from the key name (MIN_VALUE & MAX_VALUE should also not be returned)', () => {
    expect(getGetCleanedMojoEnumKeys(MockedJsonRpcErrorEnum)).toEqual([
      'ParsingError',
      'InvalidRequest',
      'MethodNotFound',
      'InvalidParams',
      'InternalError'
    ])
  })
})