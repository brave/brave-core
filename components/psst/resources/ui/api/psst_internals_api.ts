// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as Mojom from 'gen/brave/components/psst/core/common/psst_ui_common.mojom.m.js'

import {
  actionsFor,
  createInterfaceApi,
  eventsFor,
  query,
} from '$web-common/api'

export * from 'gen/brave/components/psst/core/common/psst_ui_common.mojom.m.js'

export function createPsstInternalsApi() {
  const handler = Mojom.PsstInternalsHandler.getRemote()

  // `eventsFor` assigns this synchronously while the argument object is still
  // being built, so it is set before `createInterfaceApi` runs the `entries`
  // query.
  let pageRemote: Mojom.PsstInternalsPageRemote

  const api = createInterfaceApi({
    endpoints: {
      entries: query({
        // One call both returns the buffered timeline and subscribes the page
        // to later additions, so no entry can fall between the two.
        query: () => handler.bindPage(pageRemote).then((r) => r.entries),
        prefetchWithArgs: [],
        placeholderData: [] as Mojom.PsstLogEntry[],
      }),
    },

    actions: {
      ...actionsFor(handler, ['clear']),
    },

    events: {
      ...eventsFor(
        Mojom.PsstInternalsPageInterface,
        {
          onEntriesAdded(entries: Mojom.PsstLogEntry[]) {
            api.entries.update((old = []) => [...old, ...entries])
          },
          onCleared() {
            api.entries.update([])
          },
        },
        (observer) => {
          pageRemote = new Mojom.PsstInternalsPageReceiver(
            observer,
          ).$.bindNewPipeAndPassRemote()
        },
      ),
    },
  })

  return api
}

// The queries, actions and events brave://psst-internals is built on.
export type PsstInternalsApi = ReturnType<typeof createPsstInternalsApi>
