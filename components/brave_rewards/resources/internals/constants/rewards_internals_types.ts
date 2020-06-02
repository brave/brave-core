/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  GET_REWARDS_ENABLED = '@@rewards_internals/GET_REWARDS_ENABLED',
  ON_GET_REWARDS_ENABLED = '@@rewards_internals/ON_GET_REWARDS_ENABLED',
  GET_REWARDS_INTERNALS_INFO = '@@rewards_internals/GET_REWARDS_INTERNALS_INFO',
  ON_GET_REWARDS_INTERNALS_INFO = '@@rewards_internals/ON_GET_REWARDS_INTERNALS_INFO',
  GET_BALANCE = '@@rewards_internals/GET_BALANCE',
  ON_BALANCE = '@@rewards_internals/ON_BALANCE',
  GET_PROMOTIONS = '@@rewards_internals/GET_PROMOTIONS',
  ON_PROMOTIONS = '@@rewards_internals/ON_PROMOTIONS',
  GET_LOG = '@@rewards_internals/GET_LOG',
  ON_GET_LOG = '@@rewards_internals/ON_GET_LOG',
  CLEAR_LOG = '@@rewards_internals/CLEAR_LOG',
  ON_CLEAR_LOG = '@@rewards_internals/ON_CLEAR_LOG'
}
