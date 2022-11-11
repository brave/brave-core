/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWallet } from './external_wallet'
import { compareVersionStrings } from './version_string'

export type UserType = 'unconnected' | 'connected' | 'legacy-unconnected'

export function getUserType (
  userVersion: string,
  externalWallet: ExternalWallet | null
): UserType {
  if (externalWallet) {
    return 'connected'
  }
  try {
    if (compareVersionStrings(userVersion, '2.5') < 0) {
      return 'legacy-unconnected'
    }
  } catch {
    // If `userVersion` is not a valid version string, assume that the user
    // is a new, unconnected user.
  }
  return 'unconnected'
}
