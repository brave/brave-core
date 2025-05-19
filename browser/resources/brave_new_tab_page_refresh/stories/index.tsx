/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import './sb_locale'
import { AppModelContext } from '../components/context/app_model_context'
import { createAppModel } from './sb_app_model'
import { App } from '../components/app'

export default {
  title: 'New Tab/Refresh'
}

export function NTPRefresh() {
  return (
    <AppModelContext model={createAppModel()}>
      <div style={{ position: 'absolute', inset: 0 }}>
        <App />
      </div>
    </AppModelContext>
  )
}
