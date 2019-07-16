/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// React API
import * as React from 'react'

// Types
import { NoScriptEntry } from '../../../types/other/noScriptInfo'
import {
  AllowScriptOriginsOnce,
  SetScriptBlockedCurrentState,
  SetGroupedScriptsBlockedCurrentState,
  SetAllScriptsBlockedCurrentState,
  SetFinalScriptsBlockedState
} from '../../../types/actions/shieldsPanelActions'

// Components
import {
  BlockedListItemWithOptions,
  LinkAction,
  BlockedListItemDetails,
  BlockedListItemSummary
} from '../../../components'

// Helpers
import { getHostname, stripProtocolFromUrl } from '../../../helpers/urlUtils'
import {
  getBlockScriptText,
  filterNoScriptInfoByWillBlockState,
  checkEveryItemIsBlockedOrAllowedByUser
} from '../../../helpers/noScriptUtils'

interface Props {
  noScriptInfo: Array<any>
  maybeBlock: boolean
  allowScriptOriginsOnce: AllowScriptOriginsOnce
  setScriptBlockedCurrentState: SetScriptBlockedCurrentState
  setGroupedScriptsBlockedCurrentState: SetGroupedScriptsBlockedCurrentState
  setAllScriptsBlockedCurrentState: SetAllScriptsBlockedCurrentState
  setFinalScriptsBlockedState: SetFinalScriptsBlockedState
}

export default class NoScriptList extends React.PureComponent<Props, {}> {
  getBlockScriptText = (userInteracted: boolean, maybeBlock: boolean) => {
    return getBlockScriptText(userInteracted, maybeBlock)
  }

  setBlockState (url: string) {
    this.props.setScriptBlockedCurrentState(url)
    this.props.allowScriptOriginsOnce()
  }

  setBlockStateGroup (url: string, maybeBlock: boolean) {
    this.props.setGroupedScriptsBlockedCurrentState(url, maybeBlock)
    this.props.allowScriptOriginsOnce()
  }

  getSingleScriptRow = (url: string, scriptData: NoScriptEntry, key: number, maybeBlock: boolean) => {
    return (
      <BlockedListItemWithOptions key={key}>
        <span>{stripProtocolFromUrl(url)}</span>
        <LinkAction
          disabled={scriptData.userInteracted}
          onClick={this.setBlockState.bind(this, url)}
        >
          {this.getBlockScriptText(scriptData.userInteracted, maybeBlock)}
        </LinkAction>
      </BlockedListItemWithOptions>
    )
  }

  getNestedOrSingleScriptsLoop = (modifiedNoScriptInfo: Array<any>, maybeNested: boolean, maybeBlock: boolean) => {
    return (
      modifiedNoScriptInfo.map((script: NoScriptEntry, key: number) => {
        const url = script[0]
        const noScriptData = script[1]
        if (noScriptData.willBlock !== maybeBlock) {
          return null
        }
        if (maybeNested) {
          return (
          <BlockedListItemWithOptions key={key}>
            <span>{stripProtocolFromUrl(url)}</span>
          </BlockedListItemWithOptions>
          )
        }
        return this.getSingleScriptRow(url, noScriptData, key, maybeBlock)
      })
    )
  }

  getGroupedScriptsRow = (script: NoScriptEntry, key: number, maybeBlock: boolean) => {
    const url = script[0]
    const noScriptData = script[1]
    const hasNoScriptData = filterNoScriptInfoByWillBlockState(noScriptData, maybeBlock).length >= 2
    const everyItemIsBlockedOrAllowed = checkEveryItemIsBlockedOrAllowedByUser(noScriptData, maybeBlock)

    if (hasNoScriptData) {
      return (
        <li key={key}>
          <BlockedListItemDetails>
            <BlockedListItemSummary>
              <span>{getHostname(url)}</span>
              <LinkAction
                disabled={everyItemIsBlockedOrAllowed}
                onClick={this.setBlockStateGroup.bind(this, url, maybeBlock)}
              >
                {this.getBlockScriptText(everyItemIsBlockedOrAllowed, maybeBlock)}
              </LinkAction>
            </BlockedListItemSummary>
            {this.getNestedOrSingleScriptsLoop(noScriptData, true, maybeBlock)}
          </BlockedListItemDetails>
        </li>
      )
    }
    // if script is nested but separated from the group, render a detached script
    return this.getNestedOrSingleScriptsLoop(noScriptData, false, maybeBlock)
  }

  render () {
    const { noScriptInfo, maybeBlock } = this.props
    return noScriptInfo.map((script: NoScriptEntry, key: number) => {
      const scriptData = script[1]
      const url = scriptData[0][0]
      const scriptInfo = scriptData[0][1]

      return scriptData.length > 1
        ? (
          this.getGroupedScriptsRow(script, key, maybeBlock)
        )
        : scriptInfo.willBlock === maybeBlock && (
          this.getSingleScriptRow(url, scriptInfo, key, maybeBlock)
        )
    })
  }
}
