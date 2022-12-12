// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { SerializableOriginInfo } from '../../constants/types'

export const mockOriginInfo: SerializableOriginInfo = {
  origin: {
    scheme: 'https',
    host: 'with_a_really_looooooong_site_name.fixme.uniswap.org',
    port: 443,
    nonceIfOpaque: undefined
  },
  originSpec: 'https://with_a_really_looooooong_site_name.fixme.uniswap.org',
  eTldPlusOne: 'uniswap.org'
}
