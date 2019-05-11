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
} from 'brave-ui/features/shields'

// Locale
import { getLocale } from '../../background/api/localeAPI'

// Types
import { NoScriptInfo } from '../../types/other/noScriptInfo'
import {
  AllowScriptOriginsOnce,
  ChangeNoScriptSettings,
  ChangeAllNoScriptSettings,
  SetScriptBlockedCurrentState,
  SetAllScriptsBlockedCurrentState,
  SetFinalScriptsBlockedState
} from '../../types/actions/shieldsPanelActions'

interface Props {
  favicon: string
  hostname: string
  origin: string
  name: string
  list: NoScriptInfo
  onClose?: (event?: React.MouseEvent<any>) => void
  allowScriptOriginsOnce: AllowScriptOriginsOnce
  setScriptBlockedCurrentState: SetScriptBlockedCurrentState
  setAllScriptsBlockedCurrentState: SetAllScriptsBlockedCurrentState
  setFinalScriptsBlockedState: SetFinalScriptsBlockedState
  // TODO: remove
  changeNoScriptSettings: ChangeNoScriptSettings
  changeAllNoScriptSettings: ChangeAllNoScriptSettings
}

export default class DynamicList extends React.PureComponent<Props, {}> {
  getBlockedListSize = (isBlocked: boolean) => {
    const { list } = this.props
    return Object.keys(list)
      .filter((origin: string) => list[origin].willBlock === isBlocked).length
  }

  onClickBlockOrAllowScript = (event: React.MouseEvent<HTMLButtonElement>) => {
    this.props.setScriptBlockedCurrentState(event.currentTarget.id)
  }

  onClickAllowOrBlockAll (shouldBlock: boolean) {
    this.props.setAllScriptsBlockedCurrentState(shouldBlock)
  }

  onClickApplyScriptsOnce = () => {
    const { list } = this.props
    const allOrigins = Object.keys(list)
    const allNonBlockedOrigins = allOrigins.filter(key => list[key].willBlock === false)
    this.props.allowScriptOriginsOnce(allNonBlockedOrigins)
  }

  getList = (isBlocked: boolean) => {
    const { list } = this.props
    return Object.keys(list).map((origin, index) => {
      if (list[origin].willBlock === isBlocked) {
        return null
      }
      return (
        <BlockedListItemWithOptions key={index}>
          <span title={origin}>{origin}</span>
          <LinkAction id={origin} onClick={this.onClickBlockOrAllowScript}>
            {list[origin].willBlock ? getLocale('allow') : getLocale('block')}
          </LinkAction>
        </BlockedListItemWithOptions>
      )
    })
  }

  render () {
    const { favicon, hostname, name, onClose } = this.props
    return (
      <BlockedListContent>
        <BlockedListHeader>
          <Favicon src={favicon} />
          <SiteInfoText title={hostname}>{hostname}</SiteInfoText>
        </BlockedListHeader>
        <details open={true}>
          <BlockedListSummary stats={false} onClick={onClose}>
            <ArrowUpIcon />
            <BlockedListSummaryText>{name}</BlockedListSummaryText>
          </BlockedListSummary>
          <BlockedListDynamic>
            <BlockedListItemHeader id='blocked'>
              <BlockedListItemHeaderStats>
                {this.getBlockedListSize(true)}
              </BlockedListItemHeaderStats>
              <BlockedListItemHeaderText>{getLocale('blockedScripts')}</BlockedListItemHeaderText>
              <LinkAction id='allowAll' onClick={this.onClickAllowOrBlockAll.bind(this, false)}>
                {getLocale('allowAll')}
              </LinkAction>
            </BlockedListItemHeader>
            {this.getList(false)}
            <BlockedListItemHeader id='allowed'>
              <BlockedListItemHeaderStats>
              {this.getBlockedListSize(false)}
              </BlockedListItemHeaderStats>
              <BlockedListItemHeaderText>{getLocale('allowedScripts')}</BlockedListItemHeaderText>
              <LinkAction id='blockAll' onClick={this.onClickAllowOrBlockAll.bind(this, true)}>
                {getLocale('blockAll')}
              </LinkAction>
            </BlockedListItemHeader>
            {this.getList(true)}
          </BlockedListDynamic>
        </details>
        <BlockedListFooterWithOptions>
          <LinkAction onClick={onClose}>{getLocale('cancel')}</LinkAction>
          <ShieldsButton
            onClick={this.onClickApplyScriptsOnce}
            level='primary'
            type='accent'
            text={getLocale('applyOnce')}
          />
        </BlockedListFooterWithOptions>
      </BlockedListContent>
    )
  }
}
