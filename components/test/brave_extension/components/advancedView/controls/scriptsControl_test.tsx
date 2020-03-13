/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import ScriptsControl, { Props } from '../../../../../brave_extension/extension/brave_extension/containers/advancedView/controls/scriptsControl'
import * as actionTypes from '../../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { BlockJSOptions } from '../../../../../brave_extension/extension/brave_extension/types/other/blockTypes'
import { ShieldsPanelActionTypes } from '../../../../../brave_extension/extension/brave_extension/types/actions/shieldsPanelActions'

const fakeProps: Props = {
  isBlockedListOpen: true,
  setBlockedListOpen: () => { return },
  hostname: 'brave.com',
  favicon: '',
  javascript: 'allow',
  javascriptBlocked: 0,
  noScriptInfo: {},
  actions: {
    blockJavaScript: (setting: BlockJSOptions) => ({ type: actionTypes.JAVASCRIPT_TOGGLED, setting })
  } as ShieldsPanelActionTypes
}

describe('AdvancedView ScriptsControl component', () => {
  const baseComponent = (props: Props) =>
    <ScriptsControl {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#scriptsControl').length === 1
    expect(assertion).toBe(true)
  })

  describe('scripts control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeScriptControlSwitch = jest.spyOn(fakeProps.actions, 'blockJavaScript')
      const newProps = Object.assign(fakeProps, {
        blockJavaScript: onChangeScriptControlSwitch
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockScripts').simulate('change', value)
      expect(onChangeScriptControlSwitch).toBeCalled()
    })

    it('shows number of scripts blocked', () => {
      const newProps = Object.assign(fakeProps, { javascriptBlocked: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockScriptsStat').props().children
      expect(assertion).toBe('13')
    })

    it('trim number of scripts blocked to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { javascriptBlocked: 123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockScriptsStat').props().children
      expect(assertion).toBe('99+')
    })
  })
})
