/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled from 'styled-components'

import { LocaleContext, createLocaleContextForTesting } from '../../shared/lib/locale_context'
import { ModelContext } from '../lib/model_context'
import { createModel } from './storybook_model'

import { localeStrings } from '../lib/locale_strings'
import { App } from '../components/app'

import '@brave/leo/tokens/css/variables.css'

export default {
  title: 'Rewards/Tipping'
}

const style = {
  panelFrame: styled.div`
    box-shadow: 0px 4px 13px -2px rgba(0, 0, 0, 0.35);
    border-radius: 0px 0px 16px 16px;
    overflow: hidden;
  `
}

export function TipPanel () {
  const locale = createLocaleContextForTesting(localeStrings)
  const model = createModel()

  return (
    <style.panelFrame>
      <LocaleContext.Provider value={locale}>
        <ModelContext.Provider value={model}>
          <App />
        </ModelContext.Provider>
      </LocaleContext.Provider>
    </style.panelFrame>
  )
}
