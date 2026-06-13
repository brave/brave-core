/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ImportDataType } from '../api/welcome_api'

export function ImportDataTypeName(props: { dataType: ImportDataType }) {
  switch (props.dataType) {
    case 'autofillFormData':
      return 'Autofill form data'
    case 'extensions':
      return 'Extensions'
    case 'favorites':
      return 'Bookmarks'
    case 'history':
      return 'History'
    case 'passwords':
      return 'Saved passwords'
    case 'search':
      return 'Search engines'
  }
}
