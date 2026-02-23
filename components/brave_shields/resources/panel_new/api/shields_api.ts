/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Closable,
  createInterfaceApi,
  endpointsFor,
  actionsFor,
  state,
} from '$web-common/api'

import {
  SiteBlockInfo,
  SiteSettings,
  UIHandlerInterface,
  UIHandlerRemote,
  DataHandlerInterface,
  PanelHandlerInterface,
} from 'gen/brave/components/brave_shields/core/common/brave_shields_panel.mojom.m.js'

import {
  AdBlockMode,
  FingerprintMode,
  CookieBlockMode,
  HttpsUpgradeMode,
} from 'gen/brave/components/brave_shields/core/common/shields_settings.mojom.m.js'

import { ContentSettingsType } from 'gen/components/content_settings/core/common/content_settings_types.mojom.m'

function moveArrayItems<T, U>(
  values: T[],
  from: U[],
  to: U[],
  filterPredicate: (value: T, item: U) => boolean,
) {
  for (const value of values) {
    const matches = from.filter((item) => filterPredicate(value, item))
    for (const match of matches) {
      const index = from.indexOf(match)
      if (index >= 0) {
        from.splice(index, 1)
        to.push(match)
      }
    }
  }
}

export interface LoadTimeState {
  isHttpsByDefaultEnabled: boolean
  isTorProfile: boolean
  showStrictFingerprintingMode: boolean
  isWebcompatExceptionsServiceEnabled: boolean
  isBraveForgetFirstPartyStorageFeatureEnabled: boolean
  repeatedReloadsDetected: boolean
}

export function createShieldsApi(opts: {
  dataHandler: Closable<DataHandlerInterface>
  panelHandler: Closable<PanelHandlerInterface>
  openTab: (url: string) => void
  loadTimeState: LoadTimeState
  createUIHandlerRemote?: (impl: UIHandlerInterface) => UIHandlerRemote
}) {
  const {
    dataHandler,
    panelHandler,
    openTab,
    loadTimeState,
    createUIHandlerRemote,
  } = opts

  const api = createInterfaceApi({
    endpoints: {
      ...endpointsFor(dataHandler, {
        getSiteBlockInfo: {
          response: (r) => r.siteBlockInfo,
          prefetchWithArgs: [],
        },
        getSiteSettings: {
          response: (r) => r.siteSettings,
          prefetchWithArgs: [],
        },
        areAnyBlockedElementsPresent: {
          response: (r) => r.isAvailable,
          prefetchWithArgs: [],
          placeholderData: false,
        },
        setBraveShieldsEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteBlockInfo.update({ isBraveShieldsEnabled: enabled })
          },
        },
        setAdBlockMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [AdBlockMode]) => {
            api.getSiteSettings.update({ adBlockMode: mode })
          },
        },
        setFingerprintMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [FingerprintMode]) => {
            api.getSiteSettings.update({ fingerprintMode: mode })
          },
        },
        setCookieBlockMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [CookieBlockMode]) => {
            api.getSiteSettings.update({ cookieBlockMode: mode })
          },
        },
        setHttpsUpgradeMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [HttpsUpgradeMode]) => {
            api.getSiteSettings.update({ httpsUpgradeMode: mode })
          },
        },
        setIsNoScriptsEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteSettings.update({ isNoscriptEnabled: enabled })
          },
        },
        setBraveShieldsAdBlockOnlyModeEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteBlockInfo.update((old: SiteBlockInfo) => ({
              ...old,
              isBraveShieldsEnabled: enabled ? true : old.isBraveShieldsEnabled,
              isBraveShieldsAdBlockOnlyModeEnabled: enabled,
            }))
          },
        },
        setBraveShieldsAdBlockOnlyModePromptDismissed: {
          mutationResponse: () => undefined,
          onMutate: () => {
            api.getSiteBlockInfo.update({
              showShieldsDisabledAdBlockOnlyModePrompt: false,
            })
          },
        },
        setForgetFirstPartyStorageEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteSettings.update({
              isForgetFirstPartyStorageEnabled: enabled,
            })
          },
        },
        setWebcompatEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([feature, enabled]: [ContentSettingsType, boolean]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              webcompatSettings: {
                ...old.webcompatSettings,
                [feature]: enabled,
              },
            }))
          },
        },
        allowScriptsOnce: {
          mutationResponse: () => undefined,
          onMutate: ([urls]: [string[]]) => {
            api.getSiteBlockInfo.update((old: SiteBlockInfo) => {
              const blocked = [...old.blockedJsList]
              const allowed = [...old.allowedJsList]
              moveArrayItems(urls, blocked, allowed, (url, item) => {
                return item.url.startsWith(url)
              })
              return { ...old, blockedJsList: blocked, allowedJsList: allowed }
            })
          },
        },
        blockAllowedScripts: {
          mutationResponse: () => undefined,
          onMutate: ([urls]: [string[]]) => {
            api.getSiteBlockInfo.update((old: SiteBlockInfo) => {
              const blocked = [...old.blockedJsList]
              const allowed = [...old.allowedJsList]
              moveArrayItems(urls, allowed, blocked, (url, item) => {
                return item.url.startsWith(url)
              })
              return { ...old, blockedJsList: blocked, allowedJsList: allowed }
            })
          },
        },
        resetBlockedElements: {
          mutationResponse: () => undefined,
          onMutate: () => {
            api.areAnyBlockedElementsPresent.update(false)
          },
        },
      }),

      ...endpointsFor(panelHandler, {
        getAdvancedViewEnabled: {
          response: (r) => r.isEnabled,
          prefetchWithArgs: [],
          placeholderData: false,
        },

        setAdvancedViewEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getAdvancedViewEnabled.update(enabled)
          },
        },
      }),

      isHttpsByDefaultEnabled: state(loadTimeState.isHttpsByDefaultEnabled),
      isTorProfile: state(loadTimeState.isTorProfile),
      showStrictFingerprintingMode: state(
        loadTimeState.showStrictFingerprintingMode,
      ),
      isWebcompatExceptionsServiceEnabled: state(
        loadTimeState.isWebcompatExceptionsServiceEnabled,
      ),
      isBraveForgetFirstPartyStorageFeatureEnabled: state(
        loadTimeState.isBraveForgetFirstPartyStorageFeatureEnabled,
      ),
      repeatedReloadsDetected: state(loadTimeState.repeatedReloadsDetected),
    },

    actions: {
      ...actionsFor(panelHandler, ['closeUI', 'showUI']),
      ...actionsFor(dataHandler, ['openWebCompatWindow', 'updateFavicon']),
      openTab,
    },
  })

  if (createUIHandlerRemote) {
    dataHandler.registerUIHandler(
      createUIHandlerRemote({
        onSiteBlockInfoChanged(siteBlockInfo) {
          // Reset site settings data when the host changes.
          const prevHost = api.getSiteBlockInfo.current()?.host ?? ''
          if (prevHost && prevHost !== siteBlockInfo.host) {
            api.getSiteSettings.reset()
          }

          api.getSiteBlockInfo.update(siteBlockInfo)
        },
      }),
    )
  }

  return api
}

export type ShieldsApi = ReturnType<typeof createShieldsApi>
