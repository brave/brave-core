/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import PanelEmpty from './index'

describe('PanelEmpty tests', () => {
  const baseComponent = () => <PanelEmpty id='empty' />

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#empty').length
      expect(assertion).toBe(1)
    })
  })
})
