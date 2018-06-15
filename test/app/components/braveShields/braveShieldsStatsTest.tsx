/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import BraveShieldsStats from '../../../../app/components/braveShields/braveShieldsStats'
import { BraveShieldsStatsProps } from '../../../../app/components/braveShields/braveShieldsStats'
import { shallow } from 'enzyme'
// import * as sinon from 'sinon'

const fakeProps: BraveShieldsStatsProps = {
  braveShields: 'allow',
  adsBlocked: 1,
  trackersBlocked: 2,
  httpsRedirected: 3,
  javascriptBlocked: 4,
  fingerprintingBlocked: 5,
  adsBlockedResources: [],
  trackersBlockedResources: [],
  httpsRedirectedResources: [],
  javascriptBlockedResources: [],
  fingerprintingBlockedResources: []
}

describe('BraveShieldsStats component', () => {
  const baseComponent = (props: BraveShieldsStatsProps) =>
    <BraveShieldsStats {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsStats').length === 1
    assert.equal(assertion, true)
  })

  describe('stats for ads and trackers blocked', () => {
    it('correctly displays stats info based on state of ads + trackers blocked', () => {
      const newProps = Object.assign(fakeProps, { adsBlocked: 1007, trackersBlocked: 330 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#totalAdsTrackersBlockedStat').html().includes('1337')

      assert.equal(assertion, true)
    })

    it('onClick event fires local state change for opening details', () => {
      const componentId = 'totalAdsTrackersBlockedStat'
      const target = { target: { id: componentId } }
      const wrapper = shallow(baseComponent(fakeProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion = wrapper.state('adsTrackersBlockedDetailsOpen')

      assert.equal(assertion, true)
    })

    it('onClick shows a list of blocked resources', () => {
      const componentId = 'totalAdsTrackersBlockedStat'
      const target = { target: { id: componentId } }
      const newProps = Object.assign(fakeProps, {
        adsBlockedResources: [
          'https://anthony-beats-chuck-norris-in-everything.co.uk',
          'https://in-fact-jocelyn-can-beat-anthony-very-easily.news'
        ]
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion1 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://anthony-beats-chuck-norris-in-everything.co.uk')

      // ONE CAN NOT BEAT ANTHONY. THIS TEST NEVER FAILS
      assert.equal(assertion1, true)

      // NEITHER THIS. WHAT A TEAM
      const assertion2 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://in-fact-jocelyn-can-beat-anthony-very-easily.news')

      assert.equal(assertion2, true)
    })

    it('properly merges ads + trackers in the same list', () => {
      const componentId = 'totalAdsTrackersBlockedStat'
      const target = { target: { id: componentId } }
      const newProps = Object.assign(fakeProps, {
        adsBlockedResources: ['https://george-clooney-was-born-same-day-as-cezar.biz'],
        trackersBlockedResources: ['https://this-url-is-courtesy-of-too-much-nespresso.info']
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion1 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://george-clooney-was-born-same-day-as-cezar.biz')

        assert.equal(assertion1, true)
      const assertion2 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://this-url-is-courtesy-of-too-much-nespresso.info')

      assert.equal(assertion2, true)
    })
  })

  describe('stats for https redirected', () => {
    it('correctly displays stats info based on state', () => {
      const newProps = Object.assign(fakeProps, { httpsRedirected: 444 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#httpsRedirectedStat').html().includes('444')

      assert.equal(assertion, true)
    })

    it('onClick event fires local state change for opening details', () => {
      const componentId = 'httpsRedirectedStat'
      const target = { target: { id: componentId } }
      const wrapper = shallow(baseComponent(fakeProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion = wrapper.state('httpsRedirectedDetailsOpen')
      assert.equal(assertion, true)
    })

    it('onClick shows a list of redirected resources', () => {
      const componentId = 'httpsRedirectedStat'
      const target = { target: { id: componentId } }
      const newProps = Object.assign(fakeProps, {
        httpsRedirectedResources: [
          'https://serg-knows-a-few-pokemons.biz',
          'https://serg-knows-charizard-and-all-dragon-pokemons.asia'
        ]
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion1 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://serg-knows-a-few-pokemons.biz')

      assert.equal(assertion1, true)

      const assertion2 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://serg-knows-charizard-and-all-dragon-pokemons.asia')

      assert.equal(assertion2, true)
    })
  })

  describe('stats for javascript blocked', () => {
    it('correctly displays stats info based on state', () => {
      const newProps = Object.assign(fakeProps, { javascriptBlocked: 12123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#javascriptBlockedStat').html().includes('12123')

      assert.equal(assertion, true)
    })

    it('onClick event fires local state change for opening details', () => {
      const componentId = 'javascriptBlockedStat'
      const target = { target: { id: componentId } }
      const wrapper = shallow(baseComponent(fakeProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion = wrapper.state('javascriptBlockedDetailsOpen')
      assert.equal(assertion, true)
    })

    it('onClick shows a list of blocked resources', () => {
      const componentId = 'javascriptBlockedStat'
      const target = { target: { id: componentId } }
      const newProps = Object.assign(fakeProps, {
        javascriptBlockedResources: [
          'https://brian-bondy-is-the-real-sub-zero-from-mortal-kombat.biz',
          'https://brian-clifton-is-the-real-scorpion-from-mortal-kombat.biz'
        ]
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion1 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://brian-bondy-is-the-real-sub-zero-from-mortal-kombat.biz')

      assert.equal(assertion1, true)

      const assertion2 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://brian-bondy-is-the-real-sub-zero-from-mortal-kombat.biz')

      assert.equal(assertion2, true)
    })
  })

  describe('stats for fingerprinting blocked', () => {
    it('correctly displays stats info based on state', () => {
      const newProps = Object.assign(fakeProps, { fingerprintingBlocked: 7777 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#fingerprintingBlockedStat').html().includes('7777')

      assert.equal(assertion, true)
    })

    it('onClick event fires local state change for opening details', () => {
      const componentId = 'fingerprintingBlockedStat'
      const target = { target: { id: componentId } }
      const wrapper = shallow(baseComponent(fakeProps))
      wrapper.find(`#${componentId}`).simulate('click', target)
      const assertion = wrapper.state('fingerprintingBlockedDetailsOpen')
      assert.equal(assertion, true)
    })

    it('onClick shows a list of blocked resources', () => {
      const componentId = 'fingerprintingBlockedStat'
      const target = { target: { id: componentId } }
      const newProps = Object.assign(fakeProps, {
        fingerprintingBlockedResources: [
          'https://this-fingerprinting-is-blocked.email',
          'https://this-fingerprinting-didnt.work'
        ]
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find(`#${componentId}`).simulate('click', target)

      const assertion1 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://this-fingerprinting-is-blocked.email')

      assert.equal(assertion1, true)

      const assertion2 = wrapper.find('#blockedResourcesStats')
        .html().includes('https://this-fingerprinting-didnt.work')

      assert.equal(assertion2, true)
    })
  })
})
