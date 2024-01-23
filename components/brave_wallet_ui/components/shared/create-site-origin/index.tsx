// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { getLocale } from '../../../../common/locale'

export interface Props {
  originSpec: string
  eTldPlusOne: string
}

export const CreateSiteOrigin = (props: Props) => {
  const { originSpec, eTldPlusOne } = props

  if (originSpec === 'chrome://wallet') {
    return <span>{getLocale('braveWalletPanelTitle')}</span>
  }

  if (eTldPlusOne) {
    const [before, after] = originSpec.split(eTldPlusOne)
    // Will inherit styling from parent container
    return (
      <span>
        {before}
        <b>{eTldPlusOne}</b>
        {after}
      </span>
    )
  }
  return <span>{originSpec}</span>
}

export const SiteOrigin = CreateSiteOrigin

export default CreateSiteOrigin
