/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { BrowserProfile, ImportDataType } from '../api/welcome_api'

interface Props {
  profile: BrowserProfile
  browserProfiles: Map<string, number>
  onClearSelectedProfile: () => void
  dataTypes: ImportDataType[]
  onDataTypeChanged: (type: ImportDataType, checked: boolean) => void
}

export function ImportProfileDetail(props: Props) {
  return <></>
}
