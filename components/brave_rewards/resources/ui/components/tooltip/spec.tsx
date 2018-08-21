/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import Tooltip from './index'
import { TestThemeProvider } from '../../../theme'

describe('Tooltip tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><Tooltip id='tooltip' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#tooltip').length
      expect(assertion).toBe(1)
    })
  })
})
