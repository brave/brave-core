/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Closable,
  createInterfaceApi,
  endpointsFor,
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
    key: 'shields',

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
        },
        setBraveShieldsEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteBlockInfo.update((old: SiteBlockInfo) => ({
              ...old,
              isBraveShieldsEnabled: enabled,
            }))
          },
        },
        setAdBlockMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [AdBlockMode]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              adBlockMode: mode,
            }))
          },
        },
        setFingerprintMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [FingerprintMode]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              fingerprintMode: mode,
            }))
          },
        },
        setCookieBlockMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [CookieBlockMode]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              cookieBlockMode: mode,
            }))
          },
        },
        setHttpsUpgradeMode: {
          mutationResponse: () => undefined,
          onMutate: ([mode]: [HttpsUpgradeMode]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              httpsUpgradeMode: mode,
            }))
          },
        },
        setIsNoScriptsEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              isNoscriptEnabled: enabled,
            }))
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
            api.getSiteBlockInfo.update((old: SiteBlockInfo) => ({
              ...old,
              showShieldsDisabledAdBlockOnlyModePrompt: false,
            }))
          },
        },
        setForgetFirstPartyStorageEnabled: {
          mutationResponse: () => undefined,
          onMutate: ([enabled]: [boolean]) => {
            api.getSiteSettings.update((old: SiteSettings) => ({
              ...old,
              isForgetFirstPartyStorageEnabled: enabled,
            }))
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
              const blockedJsList = [...old.blockedJsList]
              const allowedJsList = [...old.allowedJsList]
              for (const url of urls) {
                const matches = blockedJsList.filter((item) =>
                  item.url.startsWith(url),
                )
                for (const match of matches) {
                  const index = blockedJsList.indexOf(match)
                  if (index >= 0) {
                    blockedJsList.splice(index, 1)
                    allowedJsList.push(match)
                  }
                }
              }
              return { ...old, blockedJsList, allowedJsList }
            })
          },
        },
        blockAllowedScripts: {
          mutationResponse: () => undefined,
          onMutate: ([urls]: [string[]]) => {
            api.getSiteBlockInfo.update((old: SiteBlockInfo) => {
              const blockedJsList = [...old.blockedJsList]
              const allowedJsList = [...old.allowedJsList]
              for (const url of urls) {
                const matches = allowedJsList.filter((item) =>
                  item.url.startsWith(url),
                )
                for (const match of matches) {
                  const index = allowedJsList.indexOf(match)
                  if (index >= 0) {
                    allowedJsList.splice(index, 1)
                    blockedJsList.push(match)
                  }
                }
              }
              return { ...old, blockedJsList, allowedJsList }
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
        getBrowserWindowHeight: {
          response: (r) => r.height,
          prefetchWithArgs: [],
        },

        getAdvancedViewEnabled: {
          response: (r) => r.isEnabled,
          prefetchWithArgs: [],
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
      showUI: () => panelHandler.showUI(),
      closeUI: () => panelHandler.closeUI(),
      openTab,
      openWebCompatWindow: () => dataHandler.openWebCompatWindow(),
      updateFavicon: () => dataHandler.updateFavicon(),
    },
  })

  if (createUIHandlerRemote) {
    dataHandler.registerUIHandler(
      createUIHandlerRemote({
        onSiteBlockInfoChanged(siteBlockInfo) {
          api.getSiteBlockInfo.update(siteBlockInfo)
        },
      }),
    )
  }

  return api
}

export type ShieldsApi = ReturnType<typeof createShieldsApi>
