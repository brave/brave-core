// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { getLocale } from '$web-common/locale'
import { BraveWallet } from '$wallet/constants/types'

export interface Props {
  originInfo: BraveWallet.OriginInfo | undefined
}

export const CreateSiteOrigin = (props: Props) => {
  const { originInfo } = props

  if (!originInfo) {
    return <span>{getLocale(S.BRAVE_WALLET_PANEL_TITLE)}</span>
  }

  if (originInfo.eTldPlusOne) {
    const [before, after] = originInfo.originSpec.split(originInfo.eTldPlusOne)
    // Will inherit styling from parent container
    return (
      <span>
        {before}
        <span style={{ fontWeight: 600 }}>{originInfo.eTldPlusOne}</span>
        {after}
      </span>
    )
  }
  return <span>{originInfo.originSpec}</span>
}

export const SiteOrigin = CreateSiteOrigin

export default CreateSiteOrigin
