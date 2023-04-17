/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useModelState } from '../lib/model_context'

import { LoadingIcon } from '../../shared/components/icons/loading_icon'
import { CreatorView } from './creator_view'
import { TipForm } from './tip_form'
import { InfoBox } from './info_box'
import { useLocaleContext } from '../lib/locale_strings'

import * as style from './app.style'

export function App () {
  const { getString } = useLocaleContext()
  const loading = useModelState((state) => state.loading)
  const error = useModelState((state) => state.error)

  if (loading) {
    return (
      <style.root>
        <style.loading><LoadingIcon /></style.loading>
      </style.root>
    )
  }

  if (error) {
    return (
      <style.error>
        <InfoBox style='error' title={getString('unexpectedErrorTitle')}>
          {getString('unexpectedErrorText')}
          <style.errorCode>
            {error}
          </style.errorCode>
        </InfoBox>
      </style.error>
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
