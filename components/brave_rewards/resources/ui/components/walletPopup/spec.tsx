/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import WalletPopup, { Props } from './index'
import { TestThemeProvider } from 'brave-ui/theme'

const props = {
  onClose: () => null,
  userName: 'tester'
}

describe('WalletSummary tests', () => {
  const baseComponent = (props: Props) => (
    <TestThemeProvider>
      <WalletPopup id='wallet-popup' {...props} >
        <p>hi</p>
      </WalletPopup>
    </TestThemeProvider>
  )

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const component = baseComponent(props)
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent(props))
      const assertion = wrapper.find('#wallet-popup').length
      expect(assertion).toBe(1)
    })
  })
})
