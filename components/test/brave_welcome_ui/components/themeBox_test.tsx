/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { Content } from '../../../brave_welcome_ui/components/index'
import { WelcomeThemeImage } from '../../../brave_welcome_ui/components/images'
import { mockThemes } from '../../testData'
import ThemeBox from '../../../../components/brave_welcome_ui/containers/screens/themeBox'

describe('ImportBox component tests', () => {

  const mockProps: Props = {
    index: 2,
    currentScreen: 2,
    onClick: () => null,
    browserThemes: mockThemes,
    onChangeTheme: () => null
  }

  describe('ImportBox render tests', () => {
    it('renders the component DOM without crashing', () => {
      const wrapper = shallow(
        <ThemeBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          onChangeTheme={mockProps.onChangeTheme}
          browserThemes={mockProps.browserThemes}
        />)
      expect(wrapper.find(Content)).toHaveLength(1)
      expect(wrapper.contains(<WelcomeThemeImage />))
    })
  })

  describe('ImportBox interaction tests', () => {
    it('should not call the import API if no theme selected', () => {
      const mockAction = jest.fn()
      const mockEvent = {
        target: {
          value: ''
        }
      }
      const wrapper = shallow(
        <ThemeBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          onChangeTheme={mockAction}
          browserThemes={mockProps.browserThemes}
        />)
      wrapper.instance().onChangeTheme(mockEvent)
      expect(mockAction.mock.calls.length).toBe(0)
    })

    it('should call the import API if valid theme selected', () => {
      const mockAction = jest.fn()
      const mockEvent = {
        target: {
          value: 'Dark'
        }
      }
      const wrapper = shallow(
        <ThemeBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          onChangeTheme={mockAction}
          browserThemes={mockProps.browserThemes}
        />)
      wrapper.instance().onChangeTheme(mockEvent)
      expect(mockAction.mock.calls.length).toBe(1)
    })
  })

  it('should have themeSelected as true if search provider selected', () => {
    const mockEvent = {
      target: {
        value: 'Dark'
      }
    }

    const wrapper = shallow(
      <ThemeBox
        index={mockProps.index}
        currentScreen={mockProps.currentScreen}
        onClick={mockProps.onClick}
        onChangeTheme={mockProps.onChangeTheme}
        browserThemes={mockProps.browserThemes}
      />)
    wrapper.instance().onChangeTheme(mockEvent)
    expect(wrapper.state().themeSelected).toEqual(true)
  })

  it('should have themeSelected as false if no search provider selected', () => {
    const mockEvent = {
      target: {
        value: ''
      }
    }
    const wrapper = shallow(
      <ThemeBox
        index={mockProps.index}
        currentScreen={mockProps.currentScreen}
        onClick={mockProps.onClick}
        onChangeTheme={mockProps.onChangeTheme}
        browserThemes={mockProps.browserThemes}
      />)
    wrapper.instance().onChangeTheme(mockEvent)
    expect(wrapper.state().themeSelected).toEqual(false)
  })

  it('should not show a default system theme option if system does not support it', () => {
    const wrapper = shallow(
      <ThemeBox
        index={mockProps.index}
        currentScreen={mockProps.currentScreen}
        onClick={mockProps.onClick}
        onChangeTheme={mockProps.onChangeTheme}
        browserThemes={mockProps.browserThemes}
      />)
    // Placeholder, Light, Dark (3 options)
    expect(wrapper.find('option')).toHaveLength(3)
  })

  it('should show a default system theme option if system supports it', () => {
    const mockThemeProp = [
      ...mockThemes,
      {
        name: 'System',
        index: '3'
      }
    ]

    const wrapper = shallow(
      <ThemeBox
        index={mockProps.index}
        currentScreen={mockProps.currentScreen}
        onClick={mockProps.onClick}
        onChangeTheme={mockProps.onChangeTheme}
        browserThemes={mockThemeProp}
      />)
    // Placeholder, Light, Dark, System(4 options)
    expect(wrapper.find('option')).toHaveLength(4)
  })
})
