/* global jest, expect, describe, it, afterEach */
import * as React from 'react'
import { shallow } from 'enzyme'
import { create } from 'react-test-renderer'
import WalletSummary, { Props } from './index'
import { TestThemeProvider } from 'brave-ui/theme'

const props = {
  report: {
    grant: { tokens: '10.0', converted: '0.25' },
    ads: { tokens: '10.0', converted: '0.25' },
    contribute: { tokens: '10.0', converted: '0.25' },
    donation: { tokens: '2.0', converted: '0.25' },
    tips: { tokens: '19.0', converted: '5.25' },
    total: { tokens: '19.0', converted: '5.25' }
  },
  onActivity: () => { console.log('nothing') }
}

describe('WalletSummary tests', () => {
  const baseComponent = (props: Props) => (
    <TestThemeProvider>
      <WalletSummary id='empty' {...props} />
    </TestThemeProvider>
  )

  describe('basic tests', () => {
    it('matches the snapshot', () => {
      const RealDate = Date
      global.Date = class extends RealDate {
        constructor () {
          return new RealDate('2019-06-02')
        }
      }
      const component = baseComponent(props)
      const tree = create(component).toJSON()
      expect(tree).toMatchSnapshot()
      global.Date = RealDate
    })

    it('renders the component', () => {
      const wrapper = shallow(baseComponent(props))
      const assertion = wrapper.find('#empty').length
      expect(assertion).toBe(1)
    })
  })
})
