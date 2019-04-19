/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// React API
import * as React from 'react'

// Components
import NoScriptList from './noScriptList'

// Types
import { NoScriptInfoInterface } from '../../types'

// Helpers
import {
  generateNoScriptInfoDataStructure,
  filterNoScriptInfoByBlockedState,
  getBlockAllText
} from '../../helpers'

import {
  BlockedListHeader,
  BlockedListSummary,
  BlockedListContent,
  BlockedListItemHeader,
  BlockedListDynamic,
  BlockedListFooter,
  ArrowUpIcon,
  LinkAction,
  Favicon,
  SiteInfoText,
  BlockedListSummaryText,
  BlockedListItemHeaderStats,
  BlockedListItemHeaderText,
  ShieldsButton
} from '../../../../../src/features/shields'

// Helpers
import { getLocale } from '../../fakeLocale'

interface Props {
  favicon: string
  hostname: string
  name: string
  list: { [key: string]: NoScriptInfoInterface }
  onClose?: (event?: any) => void
}

export default class CoreFeature extends React.PureComponent<Props, {}> {
  get noScriptInfo () {
    return this.props.list
  }

  get generatedNoScriptData () {
    return generateNoScriptInfoDataStructure(this.noScriptInfo)
  }

  get blockedScriptsLength () {
    return filterNoScriptInfoByBlockedState(Object.entries(this.noScriptInfo), true).length
  }

  get allowedScriptsLength () {
    return filterNoScriptInfoByBlockedState(Object.entries(this.noScriptInfo), false).length
  }

  blockOrAllowAll (blockOrAllow: boolean) {
    return
  }

  setFinalScriptsBlockedState = () => {
    return
  }

  getBlockAllText (shouldBlock: boolean) {
    return getBlockAllText(this.noScriptInfo, shouldBlock)
  }

  render () {
    const { onClose } = this.props
    return (
      <BlockedListContent>
        <BlockedListHeader>
          <Favicon src={this.props.favicon} />
          <SiteInfoText>buzfeed.com</SiteInfoText>
        </BlockedListHeader>
        <details open={true}>
          <BlockedListSummary stats={false} onClick={onClose}>
            <ArrowUpIcon />
            <BlockedListSummaryText>{getLocale('scriptsOnThisSite')}</BlockedListSummaryText>
          </BlockedListSummary>
          <BlockedListDynamic>
          {
            this.blockedScriptsLength > 0 && (
              <>
                <BlockedListItemHeader id='blocked'>
                  <BlockedListItemHeaderStats>
                    {this.blockedScriptsLength}
                  </BlockedListItemHeaderStats>
                  <BlockedListItemHeaderText>{getLocale('blockedScripts')}</BlockedListItemHeaderText>
                  <LinkAction onClick={this.blockOrAllowAll.bind(this, false)}>{this.getBlockAllText(true)}</LinkAction>
                </BlockedListItemHeader>
                <NoScriptList shouldBlock={true} noScriptInfo={this.generatedNoScriptData} />
              </>
            )
          }
          {
            this.allowedScriptsLength > 0 && (
              <>
                <BlockedListItemHeader id='allowed'>
                  <BlockedListItemHeaderStats>
                    {this.allowedScriptsLength}
                  </BlockedListItemHeaderStats>
                  <BlockedListItemHeaderText>{getLocale('allowedScripts')}</BlockedListItemHeaderText>
                  <LinkAction onClick={this.blockOrAllowAll.bind(this, true)}>{this.getBlockAllText(false)}</LinkAction>
                </BlockedListItemHeader>
                <NoScriptList shouldBlock={false} noScriptInfo={this.generatedNoScriptData} />
              </>
            )
          }
          </BlockedListDynamic>
        </details>
        <BlockedListFooter>
          <ShieldsButton level='primary' type='accent' onClick={onClose} text={getLocale('goBack')} />
        </BlockedListFooter>
      </BlockedListContent>
    )
  }
}
