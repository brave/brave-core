// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from 'gen/brave/components/psst/common/psst_ui_common.mojom.m.js'

import {
  Closable,
  createInterfaceApi,
  actionsFor,
  state,
  eventsFor,
  endpointsFor,
  VoidMethodKeys,
} from '$web-common/api'

export const createPsstDialogApi = (
  consentHelper: Closable<Mojom.PsstConsentHelperInterface>,
  callbackRouter: Closable<Mojom.PsstConsentDialogInterface>,
) => {
  let consentDialogObserver: Mojom.PsstConsentDialogInterface

  const api = createInterfaceApi({
    endpoints: {},
    events: {
      ...eventsFor(
        Mojom.PsstConsentDialogInterface,
        {
          setSettingsCardData(scd) {},
          onSetCompleted(appliedChecks, errors) {},
          onSetRequestDone(url, error) {},
        },
        (observer) => {
          consentDialogObserver = observer
        },
      ),
    },
  })

  return {
    api,
    consentDialogObserver: consentDialogObserver!,
  }
}

export type PsstDialogAPI = ReturnType<typeof createPsstDialogApi>
