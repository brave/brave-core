// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

export function getListPageItems<T extends any[]>(
  items: T,
  currentPageNumber: number,
  maxItemsPerPage: number
) {
  const pageStartItemIndex =
    currentPageNumber * maxItemsPerPage - maxItemsPerPage
  return items.slice(pageStartItemIndex, pageStartItemIndex + maxItemsPerPage)
}

export function getLastPageNumber<T extends any[]>(
  items: T,
  maxItemsPerPage: number
) {
  return Math.floor(items.length / maxItemsPerPage) + 1
}
