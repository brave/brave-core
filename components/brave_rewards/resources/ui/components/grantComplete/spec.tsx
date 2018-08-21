/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import GrantComplete from './index'
import { TestThemeProvider } from '../../../theme'

describe('Grant complete tests', () => {
  const baseComponent = (props?: object) => <TestThemeProvider><GrantComplete id='complete' {...props} /></TestThemeProvider>

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#complete').length
      expect(assertion).toBe(1)
    })
  })
})
