/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as BraveNewTabPage from 'gen/brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.m.js'

// Provide access to all the generated types
export * from 'gen/brave/components/brave_new_tab_ui/brave_new_tab_page.mojom.m.js'

import { images as backgrounds } from '../data/backgrounds'

/**
 * Generates a random image for new tab backgrounds
 */
export const randomBackgroundImage = (): NewTab.BackgroundWallpaper => {
  const randomIndex: number = Math.floor(Math.random() * backgrounds.length)
  const image: NewTab.BackgroundWallpaper = Object.assign({}, backgrounds[randomIndex])
  return image
}

interface API {
  pageCallbackRouter: BraveNewTabPage.PageCallbackRouter
  pageHandler: BraveNewTabPage.PageHandlerRemote
  addCustomBackgroundUpdatedListener: (listener: CustomBackgroundUpdated) => void
}

type CustomBackgroundUpdated = (background: BraveNewTabPage.CustomBackground) => void

let ntpBrowserAPIInstance: API

class NTPBrowserAPI implements API {
  pageCallbackRouter = new BraveNewTabPage.PageCallbackRouter()
  pageHandler = new BraveNewTabPage.PageHandlerRemote()

  constructor () {
    const factory = BraveNewTabPage.PageHandlerFactory.getRemote()
    factory.createPageHandler(
      this.pageCallbackRouter.$.bindNewPipeAndPassRemote(),
      this.pageHandler.$.bindNewPipeAndPassReceiver()
    )
  }

  addCustomBackgroundUpdatedListener (listener: CustomBackgroundUpdated) {
    this.pageCallbackRouter.onBackgroundUpdated.addListener(listener)
  }
}

export default function getNTPBrowserAPI () {
  if (!ntpBrowserAPIInstance) {
    ntpBrowserAPIInstance = new NTPBrowserAPI()
  }
  return ntpBrowserAPIInstance
}
