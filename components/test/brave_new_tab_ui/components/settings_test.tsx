import * as React from 'react'
import { shallow } from 'enzyme'
import { SettingsMenu, SettingsWrapper } from 'brave-ui/features/newTab/default'
import Settings, { Props } from '../../../../components/brave_new_tab_ui/components/newTab/settings'

describe('settings component tests', () => {
  const mockProps: Props = {
    onClickOutside: () => null,
    toggleShowBackgroundImage: () => null,
    showBackgroundImage: true
  }
  it('should render the component properly', () => {
    const wrapper = shallow(
      <Settings
        onClickOutside={mockProps.onClickOutside}
        toggleShowBackgroundImage={mockProps.toggleShowBackgroundImage}
        showBackgroundImage={mockProps.showBackgroundImage}
      />)
    expect(wrapper.find(SettingsMenu)).toHaveLength(1)
    expect(wrapper.find(SettingsWrapper)).toHaveLength(1)
  })
})
