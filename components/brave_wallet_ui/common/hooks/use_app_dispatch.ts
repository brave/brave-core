// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useDispatch } from 'react-redux'

// types
import type { WalletPanelAppDispatch } from '../../panel/store'
import type { WalletPageAppDispatch } from '../../page/store'

export const useAppDispatch = () =>
  useDispatch<WalletPageAppDispatch & WalletPanelAppDispatch>()
