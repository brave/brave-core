/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { PsstProgressModal } from '../components/PsstProgressModal'
import { PsstDialogAPI } from '../api/psst_dialog_api'
import { PsstDialogAPIProvider } from '../api/psst_dialog_api_context'

export interface Props {
  apiContext: PsstDialogAPI['api']
}

export default function PsstDlgContainer(_props: Readonly<Props>) {
  return (
    <PsstDialogAPIProvider api={_props.apiContext}>
      <PsstProgressModal />
    </PsstDialogAPIProvider>
  )
}
