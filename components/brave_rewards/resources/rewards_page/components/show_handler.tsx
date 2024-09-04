/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppModelContext, useAppState } from '../lib/app_model_context'

interface Props {
  children: React.ReactNode
}

export function ShowHandler(props: Props) {
  const model = React.useContext(AppModelContext)
  const openTime = useAppState((state) => state.openTime)

  React.useEffect(() => { model.onAppRendered() }, [model, openTime])

  return <div key={`rewards-page-${openTime}`}>{props.children}</div>
}
