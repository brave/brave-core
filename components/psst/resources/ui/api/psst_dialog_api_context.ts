/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import generateReactContextForAPI from '$web-common/api/react_api'
import { type PsstDialogAPI } from './psst_dialog_api'

export const { useAPI: usePsstDialogAPI, Provider: PsstDialogAPIProvider } =
  generateReactContextForAPI<PsstDialogAPI>()
