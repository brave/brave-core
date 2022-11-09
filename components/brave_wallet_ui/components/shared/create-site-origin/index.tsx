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

const CreateSiteOrigin = (props: Props) => {
  const { originSpec, eTldPlusOne } = props

  const url = React.useMemo(() => {
    if (originSpec === 'chrome://wallet') {
      return <span>{getLocale('braveWalletPanelTitle')}</span>
    }
    if (eTldPlusOne) {
      const before = originSpec.split(eTldPlusOne)[0]
      const after = originSpec.split(eTldPlusOne)[1]
      // Will inherit styling from parent container
      return <span>{before}<b>{eTldPlusOne}</b>{after}</span>
    }
    return <span>{originSpec}</span>
  }, [eTldPlusOne, originSpec])

  return (<>{url}</>)
}
export default CreateSiteOrigin
