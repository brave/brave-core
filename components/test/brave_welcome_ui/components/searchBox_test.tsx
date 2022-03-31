/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { Content } from '../../../../components/brave_welcome_ui/components'
import { WelcomeSearchImage } from '../../../../components/brave_welcome_ui/components/images'
import SearchBox, { Props } from '../../../../components/brave_welcome_ui/containers/screens/searchBox'
import { mockSearchProviders } from '../../testData'

describe('searchBox component tests', () => {
  const mockProps: Props = {
    index: 3,
    currentScreen: 3,
    onClick: () => null,
    changeDefaultSearchProvider: () => null,
    searchProviders: mockSearchProviders
  }
  describe('searchBox render tests', () => {
    it('renders the component DOM without crashing', () => {
      const wrapper = shallow(
        <SearchBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          changeDefaultSearchProvider={mockProps.changeDefaultSearchProvider}
          searchProviders={mockProps.searchProviders}
        />)

      expect(wrapper.find(Content)).toHaveLength(1)
      expect(wrapper.contains(<WelcomeSearchImage />))
    })
  })

  describe('searchBox method tests', () => {
    describe('onChangeDefaultSearchEngine', () => {
      it('should not call API action if no provider selected', () => {
        const mockAction = jest.fn()
        const mockEvent = {
          target: {
            value: ''
          }
        }
        const wrapper = shallow(
          <SearchBox
            index={mockProps.index}
            currentScreen={mockProps.currentScreen}
            onClick={mockProps.onClick}
            changeDefaultSearchProvider={mockAction}
            searchProviders={mockProps.searchProviders}
          />)
        wrapper.instance().onChangeDefaultSearchEngine(mockEvent)
        expect(mockAction.mock.calls.length).toBe(0)
      })

      it('should call API action if search provider selected', () => {
        const mockAction = jest.fn()
        const mockEvent = {
          target: {
            value: '1'
          }
        }
        const wrapper = shallow(
          <SearchBox
            index={mockProps.index}
            currentScreen={mockProps.currentScreen}
            onClick={mockProps.onClick}
            changeDefaultSearchProvider={mockAction}
            searchProviders={mockProps.searchProviders}
          />)
        wrapper.instance().onChangeDefaultSearchEngine(mockEvent)
        expect(mockAction.mock.calls.length).toBe(1)
      })

      it('should have searchEngineSelected as true if search provider selected', () => {
        const mockEvent = {
          target: {
            value: '1'
          }
        }
        const wrapper = shallow(
          <SearchBox
            index={mockProps.index}
            currentScreen={mockProps.currentScreen}
            onClick={mockProps.onClick}
            changeDefaultSearchProvider={mockProps.changeDefaultSearchProvider}
            searchProviders={mockProps.searchProviders}
          />)
        wrapper.instance().onChangeDefaultSearchEngine(mockEvent)
        expect(wrapper.state().searchEngineSelected).toEqual(true)
      })

      it('should have searchEngineSelected as false if no search provider selected', () => {
        const mockEvent = {
          target: {
            value: ''
          }
        }
        const wrapper = shallow(
          <SearchBox
            index={mockProps.index}
            currentScreen={mockProps.currentScreen}
            onClick={mockProps.onClick}
            changeDefaultSearchProvider={mockProps.changeDefaultSearchProvider}
            searchProviders={mockProps.searchProviders}
          />)
        wrapper.instance().onChangeDefaultSearchEngine(mockEvent)
        expect(wrapper.state().searchEngineSelected).toEqual(false)
      })
    })
  })

  describe('getDefaultSearchProvider', () => {
    it('should return the default search provider entry', () => {
      const wrapper = shallow(
        <SearchBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          changeDefaultSearchProvider={mockProps.changeDefaultSearchProvider}
          searchProviders={mockProps.searchProviders}
        />)
      const expected = mockSearchProviders[0]
      const result = wrapper.instance().getDefaultSearchProvider(mockSearchProviders)
      expect(result).toEqual(expected)
    })
  })

  describe('getProviderDisplayName', () => {
    it('should return the search provider name with an default indicator', () => {
      const mockDefaultProvider = {
        ...mockSearchProviders[0],
        name: 'MockSearchEngine'
      }

      const mockCurrentProvider = {
        ...mockSearchProviders[0],
        name: 'MockSearchEngine'
      }
      const wrapper = shallow(
        <SearchBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          changeDefaultSearchProvider={mockProps.changeDefaultSearchProvider}
          searchProviders={mockProps.searchProviders}
        />)
      const expected = 'MockSearchEngine (default)'
      const result = wrapper.instance().getProviderDisplayName(mockCurrentProvider, mockDefaultProvider)

      expect(result).toEqual(expected)
    })

    it('should return the search provider name without a default indicator', () => {
      const mockDefaultProvider = {
        ...mockSearchProviders[0],
        name: 'NotMockSearchEngine'
      }

      const mockCurrentProvider = {
        ...mockSearchProviders[0],
        name: 'MockSearchEngine'
      }
      const wrapper = shallow(
        <SearchBox
          index={mockProps.index}
          currentScreen={mockProps.currentScreen}
          onClick={mockProps.onClick}
          changeDefaultSearchProvider={mockProps.changeDefaultSearchProvider}
          searchProviders={mockProps.searchProviders}
        />)
      const expected = 'MockSearchEngine'
      const result = wrapper.instance().getProviderDisplayName(mockCurrentProvider, mockDefaultProvider)

      expect(result).toEqual(expected)
    })
  })
})
