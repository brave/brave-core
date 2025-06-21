// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Note: This library is a Polyfill for the Intl.MessageFormat API. Once its
// supported in Chrome we can remove this dependency.
import { IntlMessageFormat, PrimitiveType } from 'intl-messageformat'
import { getLocale } from './locale'

export const getPluralString = (key: string, format: Record<string, PrimitiveType>) => {
  const message = new IntlMessageFormat(getLocale(key))
  return message.format(format) as string
}
