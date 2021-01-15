/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const enum types {
  GET_TOR_GENERAL_INFO = '@@tor_internals/GET_TOR_GENERAL_INFO',
  ON_GET_TOR_GENERAL_INFO = '@@tor_internals/ON_GET_TOR_GENERAL_INFO',
  GET_TOR_LOG = '@@tor_internals/GET_TOR_LOG',
  ON_GET_TOR_LOG = '@@tor_internals/ON_GET_TOR_LOG',
  ON_GET_TOR_INIT_PERCENTAGE = '@@tor_internals/ON_GET_TOR_INIT_PERCENTAGE',
  ON_GET_TOR_CIRCUIT_ESTABLISHED = '@@tor_internals/ON_GET_TOR_CIRCUIT_ESTABLISHED',
  ON_GET_TOR_CONTROL_EVENT = '@@tor_internals/ON_GET_TOR_CONTROL_EVENT'
}
