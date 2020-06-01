import * as React from 'react'
import { shallow } from 'enzyme'
import { SettingsMenu } from '../../../../components/brave_new_tab_ui/components/default'
import Settings, { Props } from '../../../../components/brave_new_tab_ui/containers/newTab/settings'

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
    toggleShowBinance: () => undefined,
    toggleBrandedWallpaperOptIn: () => undefined,
    brandedWallpaperOptIn: false,
    allowSponsoredWallpaperUI: false,
    showRewards: false,
    showBinance: false,
    binanceSupported: false
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
        toggleShowBinance={mockProps.toggleShowBinance}
        toggleBrandedWallpaperOptIn={mockProps.toggleBrandedWallpaperOptIn}
        brandedWallpaperOptIn={mockProps.brandedWallpaperOptIn}
        allowSponsoredWallpaperUI={mockProps.allowSponsoredWallpaperUI}
        showRewards={mockProps.showRewards}
        showBinance={mockProps.showBinance}
        binanceSupported={mockProps.binanceSupported}
      />)
    expect(wrapper.find(SettingsMenu)).toHaveLength(0)
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
        toggleShowBinance={mockProps.toggleShowBinance}
        toggleBrandedWallpaperOptIn={mockProps.toggleBrandedWallpaperOptIn}
        brandedWallpaperOptIn={mockProps.brandedWallpaperOptIn}
        allowSponsoredWallpaperUI={mockProps.allowSponsoredWallpaperUI}
        showRewards={mockProps.showRewards}
        showBinance={mockProps.showBinance}
        binanceSupported={mockProps.binanceSupported}
      />)
    expect(wrapper.find(SettingsMenu)).toHaveLength(1)
  })
})
