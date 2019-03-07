/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  BlockedListHeader,
  BlockedListSummary,
  BlockedListContent,
  BlockedListItemHeader,
  BlockedListDynamic,
  BlockedListItemWithOptions,
  BlockedListFooterWithOptions,
  ArrowUpIcon,
  LinkAction,
  Favicon,
  SiteInfoText,
  BlockedListSummaryText,
  BlockedListItemHeaderStats,
  BlockedListItemHeaderText,
  ShieldsButton
} from '../../../../../src/features/shields'

// Fake data
import { getLocale } from '../../fakeLocale'

interface Props {
  favicon: string
  hostname: string
  name: string
  list: any[]
  onClose?: (event?: any) => void
}

export default class DynamicList extends React.PureComponent<Props, {}> {
  getList = (list: any[], isBlocked: boolean) => {
    return list.map((item, index) => {
      if (item.blocked === isBlocked) {
        return null
      }
      return (
        <BlockedListItemWithOptions key={index}>
          <span title={item.name}>{item.name}</span>
          {
            item.blocked
              ? <LinkAction>{getLocale('block')}</LinkAction>
              : <LinkAction>{getLocale('allow')}</LinkAction>
          }
        </BlockedListItemWithOptions>
      )
    })
  }
  render () {
    const { favicon, hostname, name, list, onClose } = this.props
    return (
      <BlockedListContent>
        <BlockedListHeader>
          <Favicon src={favicon} />
          <SiteInfoText>{hostname}</SiteInfoText>
        </BlockedListHeader>
        <details open={true}>
          <BlockedListSummary stats={false} onClick={onClose}>
            <ArrowUpIcon />
            <BlockedListSummaryText>{name}</BlockedListSummaryText>
          </BlockedListSummary>
          <BlockedListDynamic>
            <BlockedListItemHeader id='blocked'>
              <BlockedListItemHeaderStats>
                {list.filter(item => item.blocked === true).length}
              </BlockedListItemHeaderStats>
              <BlockedListItemHeaderText>{getLocale('blockedScripts')}</BlockedListItemHeaderText>
              <LinkAction>{getLocale('allowAll')}</LinkAction>
            </BlockedListItemHeader>
            {this.getList(list, true)}
            <BlockedListItemHeader id='allowed'>
              <BlockedListItemHeaderStats>
                {list.filter(item => item.blocked === false).length}
              </BlockedListItemHeaderStats>
              <BlockedListItemHeaderText>{getLocale('allowedScripts')}</BlockedListItemHeaderText>
              <LinkAction>{getLocale('blockAll')}</LinkAction>
            </BlockedListItemHeader>
            {this.getList(list, false)}
          </BlockedListDynamic>
        </details>
        <BlockedListFooterWithOptions>
          <LinkAction onClick={onClose}>{getLocale('cancel')}</LinkAction>
          <ShieldsButton onClick={onClose} level='primary' type='accent' text={getLocale('applyOnce')} />
        </BlockedListFooterWithOptions>
      </BlockedListContent>
    )
  }
}
