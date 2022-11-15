// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Alert, { Props } from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('Alert tests', () => {
  const baseComponent = (props: Props) => (
    <TestThemeProvider>
      <Alert id='alert' {...props} />
    </TestThemeProvider>
  )

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent({ type: 'success' })
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent({ type: 'success' }))
      const assertion = wrapper.find('#alert').length
      expect(assertion).toBe(1)
    })
  })
})
