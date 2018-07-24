/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import DonationTable from './index'

describe('DonationTable tests', () => {
  const baseComponent = (props?: object) => <DonationTable id='table' {...props} />

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent()
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent())
      const assertion = wrapper.find('#table').length
      expect(assertion).toBe(1)
    })
  })
})
