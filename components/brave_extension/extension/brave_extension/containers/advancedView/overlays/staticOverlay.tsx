/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  BlockedListHeader,
  BlockedListSummary,
  BlockedListContent,
  BlockedListStatic,
  BlockedListItem,
  BlockedListFooter,
  ArrowUpIcon,
  Favicon,
  SiteInfoText,
  BlockedInfoRowStats,
  BlockedListSummaryText,
  ShieldsButton
} from '../../../components'

// Helpers
import { blockedResourcesSize } from '../../../helpers/shieldsUtils'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

interface Props {
  favicon: string
  hostname: string
  stats: number
  name: string
  list: Array<string>
  onClose: (event?: React.MouseEvent) => void
}

export default class StaticList extends React.PureComponent<Props, {}> {
  get statsDisplay (): string {
    const { stats } = this.props
    return blockedResourcesSize(stats)
  }

  render () {
    const { favicon, hostname, name, list, onClose } = this.props
    return (
      <BlockedListContent>
        <BlockedListHeader>
          <Favicon src={favicon} />
          <SiteInfoText title={hostname}>{hostname}</SiteInfoText>
        </BlockedListHeader>
        <details open={true}>
          <BlockedListSummary onClick={onClose}>
          <ArrowUpIcon />
          <BlockedInfoRowStats>{this.statsDisplay}</BlockedInfoRowStats>
          <BlockedListSummaryText>{name}</BlockedListSummaryText>
          </BlockedListSummary>
          <BlockedListStatic fixedHeight={true}>
            {list.map((item, index) => <BlockedListItem key={index}>{item}</BlockedListItem>)}
          </BlockedListStatic>
        </details>
        <BlockedListFooter>
          <ShieldsButton level='primary' type='accent' onClick={onClose} text={getLocale('goBack')} />
        </BlockedListFooter>
      </BlockedListContent>
    )
  }
}
