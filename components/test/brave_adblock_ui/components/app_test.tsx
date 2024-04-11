// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { adblockInitialState } from '../../testData'
import {
  mapStateToProps
} from '../../../brave_adblock_ui/components/app'

describe('adblockPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(adblockInitialState)).toEqual(adblockInitialState)
    })
  })
})
