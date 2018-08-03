/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import WalletSummary, { Props } from './index'

const props = {
  grant: {color: '#C12D7C', tokens: 10, converted: 0.25},
  ads: {color: '#C12D7C', tokens: 10, converted: 0.25},
  contribute: {color: '#9752CB', tokens: 10, converted: 0.25},
  donation: {color: '#4C54D2', tokens: 2, converted: 0.25},
  tips: {color: '#4C54D2', tokens: 19, converted: 5.25},
  grants: [
    {
      id: '1',
      tokens: 15,
      converted: 0.75
    },
    {
      id: '2',
      tokens: 10,
      converted: 0.50
    }
  ],
  onActivity: ()=>{}
}

describe('WalletSummary tests', () => {
  const baseComponent = (props: Props) => <WalletSummary id='empty' {...props} />

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent(props)
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent(props))
      const assertion = wrapper.find('#empty').length
      expect(assertion).toBe(1)
    })
  })
})
