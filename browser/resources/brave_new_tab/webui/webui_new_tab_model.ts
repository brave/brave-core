/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

 import * as mojom from 'gen/brave/browser/ui/webui/brave_new_tab/new_tab_page.mojom.m.js'

import { NewTabPageProxy } from './new_tab_page_proxy'
import { NewTabModel, BackgroundType, defaultState } from '../models/new_tab_model'
import { createStore } from '../lib/store'
import { getCurrentBackground } from '../models/backgrounds'
import { debounce } from '$web-common/debounce'

export function backgroundTypeFromMojo(type: number): BackgroundType {
  switch (type) {
    case mojom.SelectedBackgroundType.kBrave: return 'brave'
    case mojom.SelectedBackgroundType.kCustom: return 'custom'
    case mojom.SelectedBackgroundType.kSolid: return 'solid'
    case mojom.SelectedBackgroundType.kGradient: return 'gradient'
    default: return 'none'
  }
}

export function backgroundTypeToMojo(type: BackgroundType) {
  switch (type) {
    case 'brave': return mojom.SelectedBackgroundType.kBrave
    case 'custom': return mojom.SelectedBackgroundType.kCustom
    case 'solid': return mojom.SelectedBackgroundType.kSolid
    case 'gradient': return mojom.SelectedBackgroundType.kGradient
    case 'none': return mojom.SelectedBackgroundType.kSolid
  }
}

export function createNewTabModel(): NewTabModel {
  const newTabProxy = NewTabPageProxy.getInstance()
  const { handler } = newTabProxy
  const store = createStore(defaultState())

  function updateCurrentBackground() {
    store.update({
      currentBackground: getCurrentBackground(store.getState())
    })
  }

  async function updateBackgroundsEnabled() {
    const { enabled } = await handler.getBackgroundsEnabled()
    store.update({ backgroundsEnabled: enabled })
  }

  async function updateBackgroundsCustomizable() {
    const { customizable } = await handler.getBackgroundsCustomizable()
    store.update({ backgroundsCustomizable: customizable })
  }

  async function updateSponsoredImagesEnabled() {
    const { enabled } = await handler.getSponsoredImagesEnabled()
    store.update({ sponsoredImagesEnabled: enabled })
  }

  async function updateBraveBackgrounds() {
    const { backgrounds } = await handler.getBraveBackgrounds()
    store.update({
      braveBackgrounds: backgrounds.map((item) => ({ type: 'brave', ...item }))
    })
  }

  async function updateSelectedBackground() {
    const { background } = await handler.getSelectedBackground()
    if (background) {
      store.update({
        selectedBackgroundType: backgroundTypeFromMojo(background.type),
        selectedBackground: background.value
      })
    } else {
      store.update({
        selectedBackgroundType: 'none',
        selectedBackground: ''
      })
    }
  }

  async function updateCustomBackgrounds() {
    const { backgrounds } = await handler.getCustomBackgrounds()
    store.update({ customBackgrounds: backgrounds })
  }

  async function updateSponsoredImageBackground() {
    const { background } = await handler.getSponsoredImageBackground()
    store.update({
      sponsoredImageBackground:
        background ? { type: 'sponsored', ...background } : null
    })
  }

  newTabProxy.addListeners({
    onBackgroundPrefsUpdated: debounce(async () => {
      await Promise.all([
        updateCustomBackgrounds(),
        updateSelectedBackground(),
      ])
      updateCurrentBackground()
    }, 10)
  })

  async function loadData() {
    await Promise.all([
      updateBackgroundsEnabled(),
      updateBackgroundsCustomizable(),
      updateSponsoredImagesEnabled(),
      updateBraveBackgrounds(),
      updateCustomBackgrounds(),
      updateSelectedBackground(),
      updateSponsoredImageBackground()
    ])

    updateCurrentBackground()
  }

  loadData()

  return {
    getState: store.getState,

    addListener: store.addListener,

    setBackgroundsEnabled(enabled) {
      store.update({ backgroundsEnabled: enabled })
      handler.setBackgroundsEnabled(enabled)
    },

    setSponsoredImagesEnabled(enabled) {
      store.update({ sponsoredImagesEnabled: enabled })
      handler.setSponsoredImagesEnabled(enabled)
    },

    selectBackground(type, value) {
      store.update({
        selectedBackgroundType: type,
        selectedBackground: value
      })
      handler.selectBackground({ type: backgroundTypeToMojo(type), value })
    },

    async showCustomBackgroundChooser() {
      const { imagesSelected } = await handler.showCustomBackgroundChooser()
      return imagesSelected
    },

    async removeCustomBackground(background) {
      await handler.removeCustomBackground(background)
    }
  }
}
