// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Profile from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('Profile tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><Profile id='profile' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#profile').length
      expect(assertion).toBe(1)
    })
  })
})
