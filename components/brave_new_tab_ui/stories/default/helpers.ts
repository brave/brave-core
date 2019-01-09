/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { defaultTopSitesData } from './data/topSites'

export const getItems = () => {
  return defaultTopSitesData
    .map((value, key) => ({
      id: `item-${key}`,
      name: value.name,
      url: value.url,
      favicon: value.favicon,
      background: value.background
    }))
}

export const reorder = (list: any[], startIndex: number, endIndex: number) => {
  const result = Array.from(list)
  const [removed] = result.splice(startIndex, 1)
  result.splice(endIndex, 0, removed)

  return result
}

export const getRandomBackgroundData = (imagesArr: any[]) =>
  imagesArr[Math.floor(Math.random() * imagesArr.length)]
