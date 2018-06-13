/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import NoScript, { NoScriptProps } from '../../../../app/components/noScript/noScript'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'

const fakeProps: NoScriptProps = {
  blocked: true,
  noScriptInfo: {
    0: {
      actuallyBlocked: false,
      willBlock: true
    },
    1: {
      actuallyBlocked: true,
      willBlock: false
    }
  },
  onSubmit: (origins: string[]) => {
    return {
      type: actionTypes.ALLOW_SCRIPT_ORIGINS_ONCE,
      origins
    }
  },
  onChangeNoScriptSettings: (origin: string) => {
    actionTypes.CHANGE_NO_SCRIPT_SETTINGS,
    origin
  }
}

describe('NoScript component', () => {
  const baseComponent = (props: NoScriptProps) =>
    <NoScript {...props} />

  it('renders the component by default when `blocked` is true', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#noScript').length === 1
    assert.equal(assertion, true)
  })

  it('does not render the component if `blocked` is set to false', () => {
    const newProps = Object.assign(fakeProps, { blocked: false })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#noScript').length === 1
    assert.equal(assertion, false)
  })
})