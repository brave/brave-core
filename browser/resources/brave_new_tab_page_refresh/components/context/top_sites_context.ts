/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { TopSitesAPI } from '../../api/top_sites_api'
import { createContextProvider } from './api_context'

const { Provider, useActions, useState } = createContextProvider(
  React.createContext<TopSitesAPI | null>(null))

export {
  Provider as TopSitesProvider,
  useActions as useTopSitesActions,
  useState as useTopSitesState
}

