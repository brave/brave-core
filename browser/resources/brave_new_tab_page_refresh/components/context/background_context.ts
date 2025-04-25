/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { BackgroundAPI } from '../../api/backgrounds'
import { createContextProvider } from './model_context'

const { Provider, useActions, useState } = createContextProvider(
  React.createContext<BackgroundAPI | null>(null))

export {
  Provider as BackgroundProvider,
  useActions as useBackgroundActions,
  useState as useBackgroundState
}
