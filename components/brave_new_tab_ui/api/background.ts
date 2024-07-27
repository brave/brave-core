/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as BraveNewTabPage from 'gen/brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.m.js'

import { images as backgrounds, solidColorsForBackground, gradientColorsForBackground } from '../data/backgrounds'

/**
 * Generates a random image for new tab backgrounds
 */
export const randomBackgroundImage = (): NewTab.BraveBackground => {
  const randomIndex: number = Math.floor(Math.random() * backgrounds.length)
  const image: NewTab.BraveBackground = { ...backgrounds[randomIndex], random: true }
  return image
}

export const randomColorBackground = (color: string): NewTab.BackgroundWallpaper => {
  console.assert(color === BraveNewTabPage.RANDOM_SOLID_COLOR_VALUE || color === BraveNewTabPage.RANDOM_GRADIENT_COLOR_VALUE)

  const targetColors = color === BraveNewTabPage.RANDOM_SOLID_COLOR_VALUE ? solidColorsForBackground : gradientColorsForBackground
  const randomIndex: number = Math.floor(Math.random() * targetColors.length)
  const randomColor: NewTab.ColorBackground = {
    ...targetColors[randomIndex],
    random: true
  }
  return randomColor
}

interface API {
  pageCallbackRouter: BraveNewTabPage.PageCallbackRouter
  pageHandler: BraveNewTabPage.PageHandlerRemote
  newTabMetrics: BraveNewTabPage.NewTabMetricsRemote
  addBackgroundUpdatedListener: (listener: BackgroundUpdated) => void
  addCustomImageBackgroundsUpdatedListener: (listener: CustomImageBackgroundsUpdated) => void
  addSearchPromotionDisabledListener: (listener: () => void) => void
}

type BackgroundUpdated = (background: BraveNewTabPage.Background) => void
type CustomImageBackgroundsUpdated = (backgrounds: BraveNewTabPage.CustomBackground[]) => void

let ntpBrowserAPIInstance: API

class NTPBrowserAPI implements API {
  pageCallbackRouter = new BraveNewTabPage.PageCallbackRouter()
  pageHandler = new BraveNewTabPage.PageHandlerRemote()
  newTabMetrics = new BraveNewTabPage.NewTabMetricsRemote()

  constructor () {
    const factory = BraveNewTabPage.PageHandlerFactory.getRemote()
    factory.createPageHandler(
      this.pageCallbackRouter.$.bindNewPipeAndPassRemote(),
      this.pageHandler.$.bindNewPipeAndPassReceiver(),
      this.newTabMetrics.$.bindNewPipeAndPassReceiver()
    )
  }

  addBackgroundUpdatedListener (listener: BackgroundUpdated) {
    this.pageCallbackRouter.onBackgroundUpdated.addListener(listener)
  }

  addCustomImageBackgroundsUpdatedListener (listener: CustomImageBackgroundsUpdated) {
    this.pageCallbackRouter.onCustomImageBackgroundsUpdated.addListener(listener)
  }

  addSearchPromotionDisabledListener (listener: () => void) {
    this.pageCallbackRouter.onSearchPromotionDisabled.addListener(listener)
  }
}

export default function getNTPBrowserAPI () {
  if (!ntpBrowserAPIInstance) {
    ntpBrowserAPIInstance = new NTPBrowserAPI()
  }
  return ntpBrowserAPIInstance
}
