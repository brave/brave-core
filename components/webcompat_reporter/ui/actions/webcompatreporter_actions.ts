/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/webcompatreporter_types'

export const setDialogArgs = (dialogArgs: WebcompatReporter.DialogArgs) => {
  return action(types.WEBCOMPATREPORTER_SET_DIALOG_ARGS, {
    dialogArgs
  })
}

export const onSubmitReport = (details: string, contact: string) =>
  action(types.WEBCOMPATREPORTER_ON_SUBMIT_REPORT, {
    details,
    contact
  })

export const onClose = () =>
  action(types.WEBCOMPATREPORTER_ON_CLOSE)
