// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

import {
  createInterfaceApi,
  endpointsFor,
  actionsFor,
  state,
} from '$web-common/api'
export * from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

export function createPsstDialogApi(
  consentHelper: Mojom.PsstConsentHelperRemote,
  callbackRouter: Mojom.PsstConsentDialogCallbackRouter,
) {
  const api = createInterfaceApi({
    endpoints: {
      ...endpointsFor(consentHelper, {
        applyChanges: {
          mutationResponse: () => {},
        },
      }),

      settingsCardData: state<Mojom.SettingCardData | null>(null),
      requestStatus: state<{ url: string; error?: string } | null>(null),
      completionStatus: state<{
        appliedChecks?: string[]
        errors?: string[]
      } | null>(null),
    },

    actions: {
      ...actionsFor(consentHelper, ['closeDialog']),
    },
  })

  callbackRouter.setSettingsCardData.addListener(
    (settingCardData: Mojom.SettingCardData) => {
      api.settingsCardData.update(settingCardData)
    },
  )

  callbackRouter.onSetRequestDone.addListener((url: string, error?: string) => {
    api.requestStatus.update({ url, error })
  })

  callbackRouter.onSetCompleted.addListener(
    (appliedChecks?: string[], errors?: string[]) => {
      api.completionStatus.update({ appliedChecks, errors })
    },
  )

  return api
}

export type PsstDialogAPI = ReturnType<typeof createPsstDialogApi>
