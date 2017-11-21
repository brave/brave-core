/* global describe, it */

import assert from 'assert'
import * as types from '../../../app/constants/shieldsPanelTypes'
import * as actions from '../../../app/actions/shieldsPanelActions'

describe('shieldsPanelActions actions', () => {
  it('shieldsPanelDataUpdated', () => {
    const details = {
      adBlock: 'allow',
      trackingProtection: 'block',
      origin: 'https://www.brave.com',
      hostname: 'www.brave.com'
    }
    assert.deepEqual(actions.shieldsPanelDataUpdated(details), {
      type: types.SHIELDS_PANEL_DATA_UPDATED,
      details
    })
  })

  it('toggleShields', () => {
    assert.deepEqual(actions.toggleShields(), {
      type: types.TOGGLE_SHIELDS
    })
  })

  it('toggleAdBlock action', () => {
    assert.deepEqual(actions.toggleAdBlock(), {
      type: types.TOGGLE_AD_BLOCK
    })
  })

  it('toggleTrackingProtection action', () => {
    assert.deepEqual(actions.toggleTrackingProtection(), {
      type: types.TOGGLE_TRACKING_PROTECTION
    })
  })
})
