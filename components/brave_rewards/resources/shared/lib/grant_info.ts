/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type GrantType = 'ads' | 'ugp'

export interface GrantInfo {
  id: string
  type: GrantType
  amount: number
  createdAt: number | null
  expiresAt: number | null
}
