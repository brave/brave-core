/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BackgroundState,
  SelectedBackgroundType,
  defaultBackgroundStore,
  getCurrentBackground,
} from './background_store'

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
  {
    author: 'Author 3',
    imageUrl: 'chrome://background-wallpaper/three.jpg',
    link: 'https://example.com/3',
  },
]

function createState(
  overrides: Partial<BackgroundState> = {},
): BackgroundState {
  return {
    ...defaultBackgroundStore().getState(),
    initialized: true,
    backgroundsEnabled: true,
    braveBackgrounds,
    selectedBackground: {
      type: SelectedBackgroundType.kBrave,
      value: '',
    },
    backgroundRandomValue: 0,
    ...overrides,
  }
}

describe('getCurrentBackground', () => {
  it('should use a pinned Brave background when a value is selected', () => {
    const background = getCurrentBackground(
      createState({
        selectedBackground: {
          type: SelectedBackgroundType.kBrave,
          value: braveBackgrounds[1].imageUrl,
        },
        backgroundRandomValue: 0,
      }),
    )

    expect(background).toEqual({
      type: 'brave',
      ...braveBackgrounds[1],
    })
  })

  it('should exclude disabled Brave backgrounds from the random rotation', () => {
    const background = getCurrentBackground(
      createState({
        disabledBraveBackgrounds: [braveBackgrounds[0].imageUrl],
        backgroundRandomValue: 0,
      }),
    )

    expect(background).toEqual({
      type: 'brave',
      ...braveBackgrounds[1],
    })
  })

  it('should keep the current Brave background when a different image is disabled', () => {
    // randomValue 0.5 maps to index 1 in the full 3-image catalog.
    const before = getCurrentBackground(
      createState({ backgroundRandomValue: 0.5 }),
    )
    expect(before).toEqual({
      type: 'brave',
      ...braveBackgrounds[1],
    })

    const after = getCurrentBackground(
      createState({
        disabledBraveBackgrounds: [braveBackgrounds[0].imageUrl],
        backgroundRandomValue: 0.5,
      }),
    )

    expect(after).toEqual(before)
  })

  it('should fall back to the full Brave list when every image is disabled', () => {
    const background = getCurrentBackground(
      createState({
        disabledBraveBackgrounds: braveBackgrounds.map((b) => b.imageUrl),
        backgroundRandomValue: 0,
      }),
    )

    expect(background).toEqual({
      type: 'brave',
      ...braveBackgrounds[0],
    })
  })

  it('should fall back to random when a pinned Brave background is missing', () => {
    const background = getCurrentBackground(
      createState({
        selectedBackground: {
          type: SelectedBackgroundType.kBrave,
          value: 'chrome://background-wallpaper/missing.jpg',
        },
        backgroundRandomValue: 0,
      }),
    )

    expect(background).toEqual({
      type: 'brave',
      ...braveBackgrounds[0],
    })
  })

  it('should keep the current custom background when a different image is removed', () => {
    const customBackgrounds = [
      'chrome://custom-wallpaper/a.jpg',
      'chrome://custom-wallpaper/b.jpg',
      'chrome://custom-wallpaper/c.jpg',
    ]
    const before = getCurrentBackground(
      createState({
        selectedBackground: {
          type: SelectedBackgroundType.kCustom,
          value: '',
        },
        customBackgrounds,
        backgroundRotateIndex: 1,
        customBackgroundStickyUrl: customBackgrounds[1],
      }),
    )
    expect(before).toEqual({
      type: 'custom',
      imageUrl: customBackgrounds[1],
    })

    // Removing an earlier list entry used to remap rotateIndex % length.
    const after = getCurrentBackground(
      createState({
        selectedBackground: {
          type: SelectedBackgroundType.kCustom,
          value: '',
        },
        customBackgrounds: [customBackgrounds[1], customBackgrounds[2]],
        backgroundRotateIndex: 1,
        customBackgroundStickyUrl: customBackgrounds[1],
      }),
    )
    expect(after).toEqual(before)
  })

  it('should pick a new custom background when the sticky image is removed', () => {
    const remaining = [
      'chrome://custom-wallpaper/a.jpg',
      'chrome://custom-wallpaper/c.jpg',
    ]
    const background = getCurrentBackground(
      createState({
        selectedBackground: {
          type: SelectedBackgroundType.kCustom,
          value: '',
        },
        customBackgrounds: remaining,
        backgroundRotateIndex: 1,
        customBackgroundStickyUrl: 'chrome://custom-wallpaper/b.jpg',
      }),
    )

    expect(background).toEqual({
      type: 'custom',
      imageUrl: remaining[1],
    })
  })
})
