/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ShieldsFooter, { Props } from '../../../../brave_extension/extension/brave_extension/containers/advancedView/footer'
import * as tabsAPI from '../../../../brave_extension/extension/brave_extension/background/api/tabsAPI'
import { shallow } from 'enzyme'

const fakeProps: Props = {
  toggleAdvancedView: () => undefined,
  isBlockedListOpen: false
}

describe('AdvancedView Footer component', () => {
  const baseComponent = () => <ShieldsFooter {...fakeProps} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent())
    const assertion = wrapper.find('#braveShieldsFooter').length === 1
    expect(assertion).toBe(true)
  })

  describe('when edit default settings is clicked', () => {
    let spy: jest.SpyInstance
    const url = 'chrome://settings/shields'
    beforeAll(() => {
      tabsAPI.createTab({ url })
        .catch(() => {
          expect(true).toBe(false)
        })
      spy = jest.spyOn(chrome.tabs, 'create')
    })

    afterAll(() => {
      spy.mockRestore()
    })

    it('calls chrome.tab.create', () => {
      const value = {
        preventDefault: () => {
          console.log('')
        }
      }
      const wrapper = shallow(baseComponent())
      wrapper.find('#braveShieldsFooter').simulate('click', value)
      expect(spy).toBeCalled()
      expect(spy.mock.calls[0][0]).toEqual({
        url
      })
    })
  })
})
