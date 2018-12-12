import * as React from 'react'
import { shallow } from 'enzyme'
import { types } from '../../../brave_adblock_ui/constants/adblock_types'
import { adblockInitialState } from '../../testData'
import {
  AdblockPage,
  mapStateToProps,
  mapDispatchToProps
} from '../../../brave_adblock_ui/components/app'

describe('adblockPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(adblockInitialState)).toEqual(adblockInitialState)
    })
  })

  describe('mapDispatchToProps', () => {
    it('should fire statsUpdated', () => {
      const dispatch = jest.fn()

      // For the `mapDispatchToProps`, call it directly but pass in
      // a mock function and check the arguments passed in are as expected
      mapDispatchToProps(dispatch).actions.statsUpdated()
      expect(dispatch.mock.calls[0][0]).toEqual({
        type: types.ADBLOCK_STATS_UPDATED,
        meta: undefined,
        payload: undefined
      })
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
