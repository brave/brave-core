/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import generateReactContextForAPI from '$web-common/api/react_api'
import { SettingCardData, type PsstDialogAPI } from './psst_dialog_api'

type PsstDialogContextProps = {
  api: PsstDialogAPI['api']
  siteData: SettingCardData | undefined
}

export default function useProvidePsstDialogContext(
  props: PsstDialogContextProps,
) {
  return {
    api: props.api,
    siteData: props.siteData,
  }
}

export type PsstDialogContext = ReturnType<typeof useProvidePsstDialogContext>

export const { useAPI: usePsstDialogAPI, Provider: PsstDialogAPIProvider } =
  generateReactContextForAPI<PsstDialogContextProps, PsstDialogContext>(
    useProvidePsstDialogContext,
  )
