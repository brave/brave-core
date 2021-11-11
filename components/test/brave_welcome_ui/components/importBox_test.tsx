/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { Content, PrimaryButton } from '../../../../components/brave_welcome_ui/components'
import { WelcomeImportImage } from '../../../../components/brave_welcome_ui/components/images'
import ImportBox, { Props } from '../../../../components/brave_welcome_ui/containers/screens/importBox'
import { mockImportSources } from '../../testData'

describe('ImportBox component tests', () => {
  const mockProps: Props = {
    index: 2,
    currentScreen: 2,
    onClick: () => null,
    browserProfiles: mockImportSources
  }

  describe('ImportBox render tests', () => {
    it('renders the component DOM without crashing', () => {
      const wrapper = shallow(
        <ImportBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          browserProfiles={mockProps.browserProfiles}
        />)

      expect(wrapper.find(Content)).toHaveLength(1)
      expect(wrapper.contains(<WelcomeImportImage />))
    })
  })

  describe('ImportBox interaction tests', () => {
    it('should call the import API on button click', () => {
      const mockAction = jest.fn()
      const wrapper = shallow(
        <ImportBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockAction}
          browserProfiles={mockProps.browserProfiles}
        />)
      const button = wrapper.find(PrimaryButton)
      expect(button).toHaveLength(1)
      button.simulate('click')
      expect(mockAction.mock.calls.length).toBe(1)
    })
  })

  describe('onChangeImportSource', () => {
    it('should set selectedBrowserProfile to null if non-valid option is selected', () => {
      const mockEvent = {
        target: {
          value: ''
        }
      }
      const wrapper = shallow(
        <ImportBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          browserProfiles={mockProps.browserProfiles}
        />)

      const expected = null
      wrapper.instance().onChangeImportSource(mockEvent)
      expect(wrapper.state().selectedBrowserProfile).toEqual(expected)
    })

    it('should set selectedBrowserProfile to profile if valid option selected', () => {
      const mockEvent = {
        target: {
          value: '1'
        }
      }
      const wrapper = shallow(
        <ImportBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          browserProfiles={mockImportSources}
        />)

      // Chrome profile which has browser index of 1
      const expected = mockImportSources[0]
      wrapper.instance().onChangeImportSource(mockEvent)
      expect(wrapper.state().selectedBrowserProfile).toEqual(expected)
    })
  })
})
