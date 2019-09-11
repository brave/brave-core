/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import CookiesControl, { Props } from '../../../../../brave_extension/extension/brave_extension/containers/advancedView/controls/cookiesControl'
import { BlockCookiesOptions } from '../../../../../brave_extension/extension/brave_extension/types/other/blockTypes'
import * as actionTypes from '../../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'

const fakeProps: Props = {
  isBlockedListOpen: true,
  cookies: 'allow',
  blockCookies: (setting: BlockCookiesOptions) => ({ type: actionTypes.BLOCK_COOKIES, setting })
}

describe('AdvancedView CookiesControl component', () => {
  const baseComponent = (props: Props) =>
    <CookiesControl {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#cookiesControl').length === 1
    expect(assertion).toBe(true)
  })

  describe('cookie control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeCookiesControlSelectOptions = jest.spyOn(fakeProps, 'blockCookies')
      const newProps = Object.assign(fakeProps, {
        blockCookies: onChangeCookiesControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockCookies').simulate('change', value)
      expect(onChangeCookiesControlSelectOptions).toBeCalled()
    })
  })
})
