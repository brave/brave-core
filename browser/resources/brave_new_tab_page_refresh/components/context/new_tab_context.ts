/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { NewTabAPI } from '../../api/new_tab'
import { createContextProvider } from './api_context'

const { Provider, useActions, useState } = createContextProvider(
  React.createContext<NewTabAPI | null>(null))

export {
  Provider as NewTabProvider,
  useActions as useNewTabActions,
  useState as useNewTabState
}
