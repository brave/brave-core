// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { AccountsTabState } from '../../page/reducers/accounts-tab-reducer'

export const mockAccountsTabState: AccountsTabState = {
  accountModalType: 'deposit',
  selectedAccount: undefined,
  showAccountModal: false,
  accountToRemove: undefined
}
