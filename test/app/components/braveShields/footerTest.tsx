/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ShieldsFooter from '../../../../app/components/braveShields/footer'
import * as tabsAPI from '../../../../app/background/api/tabsAPI'
import { shallow } from 'enzyme'

describe('ShieldsFooter component', () => {
  const baseComponent = () => <ShieldsFooter />

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
      spy = jest.spyOn(chrome.tabs, 'create')
    })

    afterAll(() => {
      spy.mockRestore()
    })

    it('calls chrome.tab.create', () => {
      const value = { preventDefault: () => {} }
      const wrapper = shallow(baseComponent())
      wrapper.find('#braveShieldsFooter').simulate('click', value)
      expect(spy).toBeCalled()
      expect(spy.mock.calls[0][0]).toEqual({
        url
      })
    })
  })
})
