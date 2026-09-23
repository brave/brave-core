/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ImportDataType } from '../api/welcome_api'
import { getString } from '../lib/strings'

export function ImportDataTypeName(props: { dataType: ImportDataType }) {
  switch (props.dataType) {
    case 'autofillFormData':
      return getString('WELCOME_PAGE_IMPORT_DATA_TYPE_AUTOFILL')
    case 'extensions':
      return getString('WELCOME_PAGE_IMPORT_DATA_TYPE_EXTENSIONS')
    case 'favorites':
      return getString('WELCOME_PAGE_IMPORT_DATA_TYPE_FAVORITES')
    case 'history':
      return getString('WELCOME_PAGE_IMPORT_DATA_TYPE_HISTORY')
    case 'passwords':
      return getString('WELCOME_PAGE_IMPORT_DATA_TYPE_PASSWORDS')
    case 'search':
      return getString('WELCOME_PAGE_IMPORT_DATA_TYPE_SEARCH')
  }
}
