/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// React API
import * as React from 'react'

// Components
import {
  BlockedListItemDetails,
  BlockedListItemSummary,
  BlockedListItemWithOptions,
  LinkAction
} from '../../../../../src/features/shields'

// Types
import { NoScriptInfoInterface } from '../../types'

// Helpers
import {
  getBlockScriptText,
  filterNoScriptInfoByBlockedState,
  getHostname,
  checkEveryItemIsBlockedOrAllowed,
  stripProtocolFromUrl
} from '../../helpers'

interface Props {
  noScriptInfo: Array<any>
  shouldBlock: boolean
}

export default class NoScriptList extends React.PureComponent<Props, {}> {
  get noScriptInfo () {
    return this.props.noScriptInfo
  }

  get shouldBlock () {
    return this.props.shouldBlock
  }

  setBlockState (url: string, maybeBlock: boolean) {
    return
  }

  setBlockStateGroup (hostname: string, maybeBlock: boolean) {
    return
  }

  getBlockScriptText = (haveUserInteracted: boolean, shouldBlock: boolean) => {
    return getBlockScriptText(haveUserInteracted, shouldBlock)
  }

  getSingleScriptRow = (url: string, scriptData: NoScriptInfoInterface, key: number, shouldBlock: boolean) => {
    return (
      <BlockedListItemWithOptions key={key}>
        <span>{stripProtocolFromUrl(url)}</span>
        <LinkAction disabled={scriptData.userInteracted} onClick={this.setBlockState.bind(this, url, shouldBlock)}>
          {this.getBlockScriptText(scriptData.userInteracted, shouldBlock)}
        </LinkAction>
      </BlockedListItemWithOptions>
    )
  }

  getGroupedOrDetachedScriptsLoop = (nestedScriptInfo: Array<any>, shouldBlock: boolean) => {
    return (
      nestedScriptInfo.map((nestedScript: Array<any>, nestedKey: number) => {
        const nestedScriptInfoUrl = nestedScript[0]
        const nestedScriptInfoUrlData = nestedScript[1]
        if (nestedScriptInfoUrlData.willBlock !== shouldBlock) {
          return null
        }
        return this.getSingleScriptRow(nestedScriptInfoUrl, nestedScriptInfoUrlData, nestedKey, shouldBlock)
      })
    )
  }

  getGroupedScriptsRow = (script: Array<any>, key: number, shouldBlock: boolean) => {
    const urlWithNestedScriptInfo = getHostname(script[0])
    const nestedScriptInfo = script[1]
    const hasNestedScriptInfo = filterNoScriptInfoByBlockedState(nestedScriptInfo, shouldBlock).length >= 2
    const everyItemIsBlockedOrAllowed = checkEveryItemIsBlockedOrAllowed(nestedScriptInfo, shouldBlock)

    if (hasNestedScriptInfo) {
      return (
        <li key={key}>
          <BlockedListItemDetails>
            <BlockedListItemSummary>
              <span>{urlWithNestedScriptInfo}</span>
              <LinkAction
                disabled={everyItemIsBlockedOrAllowed}
                onClick={this.setBlockStateGroup.bind(this, urlWithNestedScriptInfo, shouldBlock)}>
                {this.getBlockScriptText(everyItemIsBlockedOrAllowed, shouldBlock)}
              </LinkAction>
            </BlockedListItemSummary>
            {this.getGroupedOrDetachedScriptsLoop(nestedScriptInfo, shouldBlock)}
          </BlockedListItemDetails>
        </li>
      )
    }
    // if script is nested but separated from the group, render a detached script
    return this.getGroupedOrDetachedScriptsLoop(nestedScriptInfo, shouldBlock)
  }

  render () {
    return this.noScriptInfo.map((script: Array<any>, key: number) => {
      const scriptData = script[1]
      const url = scriptData[0][0]
      const scriptInfo = scriptData[0][1]

      return scriptData.length > 1
        ? (
          this.getGroupedScriptsRow(script, key, this.shouldBlock)
        )
        : scriptInfo.willBlock === this.shouldBlock && (
          this.getSingleScriptRow(url, scriptInfo, key, this.shouldBlock)
        )
    })
  }
}
