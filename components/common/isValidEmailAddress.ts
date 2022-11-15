// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

const ADDRESS_SEPARATORS = [';', ',']

/**
 * Very simple email address validation that determines if something looks like
 * an email address and does not look like multiple email addresses.
 */
export default function isValidEmailAddress (input: string): boolean {
  // Empty string is not a valid address
  if (!input) {
    return false
  }
  // Only a single @ symbol is valid
  const parts = input.split('@')
  if (parts.length !== 2) {
    return false
  }
  // Make sure it's only a single address, and not breaking out to also
  // include an invalid address
  if (ADDRESS_SEPARATORS.some(sep => input.includes(sep))) {
    return false
  }
  return true
}
