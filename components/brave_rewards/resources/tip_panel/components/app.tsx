/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { ModelContext, useModelState } from '../lib/model_context'

import { LoadingIcon } from '../../shared/components/icons/loading_icon'
import { CreatorView } from './creator_view'
import { TipForm } from './tip_form'

import * as style from './app.style'

export function App () {
  const model = React.useContext(ModelContext)
  const loading = useModelState((state) => state.loading)

  React.useEffect(() => {
    if (!loading) {
      model.onInitialRender()
    }
  }, [model, loading])

  if (loading) {
    return (
      <style.root>
        <style.loading><LoadingIcon /></style.loading>
      </style.root>
    )
  }

  return (
    <style.root>
      <style.creator>
        <CreatorView />
      </style.creator>
      <style.form>
        <TipForm />
      </style.form>
    </style.root>
  )
}
