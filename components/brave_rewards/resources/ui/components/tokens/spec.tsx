/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Tokens from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('Tokens tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><Tokens id={'tokens'} value={'10'} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#tokens').length
      expect(assertion).toBe(1)
    })
  })
})
