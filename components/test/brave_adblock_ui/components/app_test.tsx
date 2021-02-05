import * as React from 'react'
import { shallow } from 'enzyme'
import { adblockInitialState } from '../../testData'
import {
  AdblockPage,
  mapStateToProps
} from '../../../brave_adblock_ui/components/app'

describe('adblockPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(adblockInitialState)).toEqual(adblockInitialState)
    })
  })

  describe('adblockPage dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <AdblockPage
          actions={{}}
          adblockData={adblockInitialState.adblockData as AdBlock.State}
        />
      )
      const assertion = wrapper.find('#adblockPage')
      expect(assertion.length).toBe(1)
    })
  })
})
