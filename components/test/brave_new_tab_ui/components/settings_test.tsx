// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Note: this needs to be imported before any Settings components, to ensure the mock is set up.
import '../../../../components/brave_new_tab_ui/stories/default/data/mockBraveNewsController'

import { shallow } from 'enzyme'
import Settings, { Props, SettingsDialog } from '../../../../components/brave_new_tab_ui/containers/newTab/settings'

describe('settings component tests', () => {
  const mockProps: Props = {
    textDirection: 'ltr',
    onClickOutside: () => null,
    showSettingsMenu: false,
    toggleShowBackgroundImage: () => null,
    showBackgroundImage: true,
    toggleShowClock: () => null,
    toggleShowStats: () => null,
    toggleShowTopSites: () => null,
    showClock: true,
    showStats: true,
    showTopSites: true,
    toggleShowRewards: () => undefined,
    toggleBrandedWallpaperOptIn: () => undefined,
    brandedWallpaperOptIn: false,
    allowBackgroundCustomization: false,
    showRewards: false
  }

  it('should not render the settings menu', () => {
    const wrapper = shallow(
      <Settings
        textDirection={mockProps.textDirection}
        onClickOutside={mockProps.onClickOutside}
        showSettingsMenu={mockProps.showSettingsMenu}
        toggleShowBackgroundImage={mockProps.toggleShowBackgroundImage}
        showBackgroundImage={mockProps.showBackgroundImage}
        toggleShowClock={mockProps.toggleShowClock}
        toggleShowStats={mockProps.toggleShowStats}
        toggleShowTopSites={mockProps.toggleShowTopSites}
        showClock={mockProps.showClock}
        showStats={mockProps.showStats}
        showTopSites={mockProps.showTopSites}
        toggleShowRewards={mockProps.toggleShowRewards}
        toggleBrandedWallpaperOptIn={mockProps.toggleBrandedWallpaperOptIn}
        brandedWallpaperOptIn={mockProps.brandedWallpaperOptIn}
        allowBackgroundCustomization={mockProps.allowBackgroundCustomization}
        showRewards={mockProps.showRewards}
      />)

    expect(wrapper.find(SettingsDialog).prop('isOpen')).toBe(false)
  })

  it('should render the setting menu properly', () => {
    const wrapper = shallow(
      <Settings
        textDirection={mockProps.textDirection}
        onClickOutside={mockProps.onClickOutside}
        showSettingsMenu={true}
        toggleShowBackgroundImage={mockProps.toggleShowBackgroundImage}
        showBackgroundImage={mockProps.showBackgroundImage}
        toggleShowClock={mockProps.toggleShowClock}
        toggleShowStats={mockProps.toggleShowStats}
        toggleShowTopSites={mockProps.toggleShowTopSites}
        showClock={mockProps.showClock}
        showStats={mockProps.showStats}
        showTopSites={mockProps.showTopSites}
        toggleShowRewards={mockProps.toggleShowRewards}
        toggleBrandedWallpaperOptIn={mockProps.toggleBrandedWallpaperOptIn}
        brandedWallpaperOptIn={mockProps.brandedWallpaperOptIn}
        allowBackgroundCustomization={mockProps.allowBackgroundCustomization}
        showRewards={mockProps.showRewards}
      />)

      expect(wrapper.find(SettingsDialog).prop('isOpen')).toBe(true)
    })
})
