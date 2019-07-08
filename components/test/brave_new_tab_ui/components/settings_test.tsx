import * as React from 'react'
import { shallow } from 'enzyme'
import { SettingsMenu, SettingsWrapper } from '../../../../components/brave_new_tab_ui/components/default'
import Settings, { Props } from '../../../../components/brave_new_tab_ui/containers/newTab/settings'

describe('settings component tests', () => {
  const mockProps: Props = {
    onClickOutside: () => null,
    toggleShowBackgroundImage: () => null,
    showBackgroundImage: true,
    toggleShowClock: () => null,
    toggleShowStats: () => null,
    toggleShowTopSites: () => null,
    showClock: true,
    showStats: true,
    showTopSites: true
  }

  it('should render the component properly', () => {
    const wrapper = shallow(
      <Settings
        onClickOutside={mockProps.onClickOutside}
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
    expect(wrapper).toMatchSnapshot()
  })
})
