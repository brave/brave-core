/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Hero from './index'
import { TestThemeProvider } from 'brave-ui/theme'

describe('Hero tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><Hero id='test-hero' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#test-hero').length
      expect(assertion).toBe(1)
    })
  })
})
