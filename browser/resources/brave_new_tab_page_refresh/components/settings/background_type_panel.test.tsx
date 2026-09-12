/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { fireEvent, render, screen } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { BackgroundContext } from '../../context/background_context'
import {
  BackgroundState,
  SelectedBackgroundType,
} from '../../state/background_store'
import { BackgroundTypePanel } from './background_type_panel'

const braveBackgrounds = [
  {
    author: 'Author 1',
    imageUrl: 'chrome://background-wallpaper/one.jpg',
    link: 'https://example.com/1',
  },
  {
    author: 'Author 2',
    imageUrl: 'chrome://background-wallpaper/two.jpg',
    link: 'https://example.com/2',
  },
]

function createStore(
  state: Partial<BackgroundState>,
  actions: Partial<BackgroundState['actions']> = {},
) {
  return createStateStore<BackgroundState>({
    initialized: true,
    backgroundsEnabled: true,
    backgroundsCustomizable: true,
    sponsoredImagesEnabled: false,
    braveBackgrounds,
    customBackgrounds: [],
    customBackgroundStickyUrl: null,
    disabledBraveBackgrounds: [],
    selectedBackground: {
      type: SelectedBackgroundType.kBrave,
      value: '',
    },
    backgroundRotateIndex: 0,
    backgroundRandomValue: 0,
    sponsoredImageBackground: null,
    sponsoredRichMediaBaseUrl: '',
    actions: {
      setBackgroundsEnabled() {},
      setSponsoredImagesEnabled() {},
      selectBackground() {},
      async showCustomBackgroundChooser() {
        return false
      },
      async removeCustomBackground() {},
      setBraveBackgroundEnabled() {},
      notifySponsoredImageLoadError() {},
      notifySponsoredImageLogoClicked() {},
      notifySponsoredRichMediaEvent() {},
      ...actions,
    },
    ...state,
  })
}

function renderPanel(
  state: Partial<BackgroundState> = {},
  actions: Partial<BackgroundState['actions']> = {},
) {
  const store = createStore(state, actions)
  render(
    <BackgroundContext.Provider value={store}>
      <BackgroundTypePanel
        backgroundType={SelectedBackgroundType.kBrave}
        renderUploadOption={() => null}
      />
    </BackgroundContext.Provider>,
  )
  return store
}

describe('BackgroundTypePanel Brave backgrounds', () => {
  it('should show a description about rotation and disabling images', () => {
    renderPanel()
    expect(
      screen.getByText('NEW_TAB_BRAVE_BACKGROUNDS_DESCRIPTION'),
    ).toBeInTheDocument()
  })

  it('should not show the refresh toggle', () => {
    renderPanel()
    expect(
      screen.queryByText('NEW_TAB_RANDOMIZE_BACKGROUND_LABEL'),
    ).not.toBeInTheDocument()
    expect(document.querySelector('leo-toggle')).toBeNull()
  })

  it('should not select a Brave background when its preview is clicked', () => {
    const selectBackground = jest.fn()
    renderPanel({}, { selectBackground })

    // Brave tiles are not selection buttons — clicking the preview area should
    // not change the selected background.
    fireEvent.click(document.querySelectorAll('.preview')[0])
    expect(selectBackground).not.toHaveBeenCalled()
  })

  it('should disable a Brave background when the eye button is clicked', () => {
    const setBraveBackgroundEnabled = jest.fn()
    renderPanel({}, { setBraveBackgroundEnabled })

    fireEvent.click(
      screen.getAllByRole('button', {
        name: 'NEW_TAB_DISABLE_BACKGROUND_LABEL',
      })[0],
    )

    expect(setBraveBackgroundEnabled).toHaveBeenCalledWith(
      braveBackgrounds[0].imageUrl,
      false,
    )
  })

  it('should not allow disabling the last enabled Brave background', () => {
    const setBraveBackgroundEnabled = jest.fn()
    renderPanel(
      {
        disabledBraveBackgrounds: [braveBackgrounds[0].imageUrl],
      },
      { setBraveBackgroundEnabled },
    )

    const disableButton = screen.getByRole('button', {
      name: 'NEW_TAB_DISABLE_BACKGROUND_LABEL',
    })
    expect(disableButton).toBeDisabled()

    fireEvent.click(disableButton)
    expect(setBraveBackgroundEnabled).not.toHaveBeenCalled()
  })

  it('should re-enable a disabled Brave background when the eye button is clicked', () => {
    const setBraveBackgroundEnabled = jest.fn()
    renderPanel(
      {
        disabledBraveBackgrounds: [braveBackgrounds[0].imageUrl],
      },
      { setBraveBackgroundEnabled },
    )

    fireEvent.click(
      screen.getByRole('button', { name: 'NEW_TAB_ENABLE_BACKGROUND_LABEL' }),
    )

    expect(setBraveBackgroundEnabled).toHaveBeenCalledWith(
      braveBackgrounds[0].imageUrl,
      true,
    )
  })
})
