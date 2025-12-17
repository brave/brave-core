/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale } from '../lib/locale_strings'

const LocaleContext = React.createContext<Locale | null>(null)

export const LocaleProvider = LocaleContext.Provider

export function useLocale(): Locale {
  const locale = React.useContext(LocaleContext)
  return locale ?? { getString: () => '' }
}
