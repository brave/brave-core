// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Types
import { BraveWallet } from '../../../constants/types'
import { WalletApiEndpointBuilderParams } from '../api-base.slice'

// Utils
import { handleEndpointError } from '../../../utils/api-utils'

export const p3aEndpoints = ({
  mutation,
  query
}: WalletApiEndpointBuilderParams) => {
  return {
    reportOnboardingAction: mutation<true, BraveWallet.OnboardingAction>({
      queryFn: async (arg, { endpoint }, extraOptions, baseQuery) => {
        try {
          const { data: api } = baseQuery(undefined)
          api.braveWalletP3A.reportOnboardingAction(arg)
          return {
            data: true
          }
        } catch (error) {
          return handleEndpointError(
            endpoint,
            'Unable to report onboarding action',
            error
          )
        }
      }
    })
  }
}
