/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { images as backgrounds } from '../data/backgrounds'

/**
 * Generates a random image for new tab backgrounds
 */
export const randomBackgroundImage = (): NewTab.BackgroundWallpaper => {
  const randomIndex: number = Math.floor(Math.random() * backgrounds.length)
  const image: NewTab.BackgroundWallpaper = Object.assign({}, backgrounds[randomIndex])
  return image
}
