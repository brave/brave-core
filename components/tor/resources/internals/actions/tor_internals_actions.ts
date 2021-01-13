/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/tor_internals_types'

export const getTorGeneralInfo = () => action(types.GET_TOR_GENERAL_INFO)

export const onGetTorGeneralInfo = (generalInfo: TorInternals.GeneralInfo) =>
  action(types.ON_GET_TOR_GENERAL_INFO, {
    generalInfo
  })

export const getTorLog = () => action(types.GET_TOR_LOG)

export const onGetTorLog = (log: string) =>
  action(types.ON_GET_TOR_LOG, {
    log
  })
