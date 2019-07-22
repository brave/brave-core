import * as React from 'react'
import { shallow } from 'enzyme'
import { SettingsMenu, SettingsWrapper } from '../../../../components/brave_new_tab_ui/components/default'
import Settings, { Props } from '../../../../components/brave_new_tab_ui/containers/newTab/settings'

describe('settings component tests', () => {
  const mockProps: Props = {
    textDirection: 'ltr',
    onClick: () => null,
    onClickOutside: () => null,
    showSettingsMenu: false,
    toggleShowBackgroundImage: () => null,
    showBackgroundImage: true,
    toggleShowClock: () => null,
    toggleShowStats: () => null,
    toggleShowTopSites: () => null,
    showClock: true,
    showStats: true,
    showTopSites: true
  }

  it('should not render the settings menu', () => {
    const wrapper = shallow(
      <Settings
        textDirection={mockProps.textDirection}
        onClick={mockProps.onClick}
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
      />)
    expect(wrapper.find(SettingsMenu)).toHaveLength(0)
    expect(wrapper.find(SettingsWrapper)).toHaveLength(1)
  })

  it('should render the setting menu properly', () => {
    const wrapper = shallow(
      <Settings
        textDirection={mockProps.textDirection}
        onClick={mockProps.onClick}
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
      />)
    expect(wrapper.find(SettingsMenu)).toHaveLength(1)
    expect(wrapper.find(SettingsWrapper)).toHaveLength(1)
  })
})
