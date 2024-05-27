// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'
import Context from './Context'
import Layout from './components/Layout'

setIconBasePath('//resources/brave-icons')

createRoot(document.querySelector('#root')!).render(<Context>
  <Layout />
</Context>)
