// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { UIState } from '../../constants/types'
import { mockedErc20ApprovalTransaction } from './mock-transaction-info'

export const mockUiState: UIState = {
  transactionProviderErrorRegistry: {},
  selectedPendingTransactionId: mockedErc20ApprovalTransaction.id
}
