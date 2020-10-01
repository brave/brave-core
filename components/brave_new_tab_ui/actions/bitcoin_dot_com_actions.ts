// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { action } from 'typesafe-actions'
import { types } from '../constants/bitcoin_dot_com_types'

export const buyBitcoinDotComCrypto = () => action(types.BUY_BITCOIN_DOT_COM_CRYPTO)

export const interactionBitcoinDotCom = () => action(types.INTERACTION_BITCOIN_DOT_COM)
