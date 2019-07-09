/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  BlockedListHeader,
  BlockedListContent,
  BlockedListFooter,
  Favicon,
  ShieldsButton
} from 'brave-ui/features/shields'

// Locale
import { getLocale } from '../../background/api/localeAPI'

interface Props {
  favicon: string
  onClose?: (event?: React.MouseEvent<any>) => void
}

export default class SpeedReader extends React.PureComponent<Props, {}> {

  render () {
    const { favicon, onClose } = this.props
    return (
      <BlockedListContent>
        <BlockedListHeader>
          <Favicon src={favicon} />
        </BlockedListHeader>
        <BlockedListFooter>
          <ShieldsButton level='primary' type='accent' onClick={onClose} text={getLocale('goBack')} />
        </BlockedListFooter>
      </BlockedListContent>
    )
  }
}

