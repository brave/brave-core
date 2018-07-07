/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const types = require('../constants/welcome_types')

export const importNowRequested = () => ({
  type: types.IMPORT_NOW_REQUESTED
})

export const goToPageRequested = (pageIndex) => ({
  type: types.GO_TO_PAGE_REQUESTED,
  pageIndex
})
