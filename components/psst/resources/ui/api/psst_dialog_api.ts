// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

import {
  createInterfaceApi,
  endpointsFor,
  eventsFor,
  actionsFor,
} from '$web-common/api'
export * from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

export function createPsstDialogApi(
  consentHelper: Mojom.PsstConsentHelperInterface,
) {
  let dialogHandler: Mojom.PsstConsentDialogInterface

  const api = createInterfaceApi({
    endpoints: {
      ...endpointsFor(consentHelper, {
        performPrivacyTuning: {
          mutationResponse: () => {},
        },
      }),
    },

    actions: {
      ...actionsFor(consentHelper, ['closeDialog', 'reportFailedContent']),
    },
    events: {
      ...eventsFor(
        Mojom.PsstConsentDialogInterface,
        {
          onSetRequestStatus(url: string, error: string) {},
        },
        (observer) => {
          dialogHandler = observer
        },
      ),
    },
  })

  return {
    api,
    dialogHandler: dialogHandler!,
  }
}

/**
 * PSST Dialog API interface.
 */
export type PsstDialogAPI = ReturnType<typeof createPsstDialogApi>
