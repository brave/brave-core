// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import DataContext from '../../state/context'
import { getLocale } from '../../../../../common/locale'
import TreeNode from './tree-node'
import {
  ViewType,
  ResourceInfo,
  ResourceType,
  ResourceState
} from '../../state/component_types'
import Button from '$web-components/button'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { ScriptsInfo, Footer, ScriptsList } from './style'


interface ResourceList {
  allowedList: ResourceInfo[]
  blockedList: ResourceInfo[]
}
interface Props {
  resourcesList: ResourceList
  type: ResourceType
  totalAllowedTitle: string
  totalBlockedTitle: string
}

function groupByOrigin (data: ResourceInfo[]) {
  const map: Map<string, ResourceInfo[]> = new Map()

  const includesDupeOrigin = (searchOrigin: string) => {
    const results = data.map(entry => new URL(entry.url.url).origin)
      .filter(entry => entry.includes(searchOrigin))
    return results.length > 1
  }

  data.forEach(entry => {
    const url = new URL(entry.url.url)
    const origin = url.origin
    const items = map.get(origin)

    if (items) {
      items.push(entry)
      return // continue
    }

    // If the origin's full url is the resource itself then we show the full url as parent
    map.set(includesDupeOrigin(origin) ? origin : url.href.replace(/\/$/, ''), [])
  })

  return map
}

function getScriptsOriginsWithState (data: ResourceInfo[],
                                     state: ResourceState): string[] {
  return [...new Set(data.filter(e => e.state === state)
    .map(e => new URL(e.url.url).origin))]
}

function TreeList (props: Props) {
  const { siteBlockInfo, setViewType } = React.useContext(DataContext)

  const allowedScriptsByOrigin = React.useMemo(() =>
    groupByOrigin(props.resourcesList.allowedList),
                  [props.resourcesList.allowedList])

  const blockedScriptsByOrigin = React.useMemo(() =>
    groupByOrigin(props.resourcesList.blockedList),
                  [props.resourcesList.blockedList])

  const handleAllowAllScripts = () => {
    const origins =
      getScriptsOriginsWithState(props.resourcesList.blockedList,
                                 ResourceState.Blocked)
    getPanelBrowserAPI().dataHandler.allowScriptsOnce(origins)
  }

  const handleBlockAllScripts = () => {
    const origins =
      getScriptsOriginsWithState(props.resourcesList.allowedList,
                                 ResourceState.AllowedOnce)
    getPanelBrowserAPI().dataHandler.blockAllowedScripts(origins)
  }

  const handleBlockScript = (name: string, state: ResourceState) => {
    getPanelBrowserAPI().dataHandler.blockAllowedScripts([new URL(name).origin])
  }

  const handleAllowScript = (name: string, state: ResourceState) => {
    getPanelBrowserAPI().dataHandler.allowScriptsOnce([new URL(name).origin])
  }

  const isAllowedSectionVisible = props.type === ResourceType.Script
  const permissionButtonHandler =
    isAllowedSectionVisible ? handleAllowScript : undefined
  return (
    <S.Box>
      <S.HeaderBox>
        <S.SiteTitleBox>
          <S.FavIconBox>
            <img src={siteBlockInfo?.faviconUrl.url} />
          </S.FavIconBox>
          <S.SiteTitle>{siteBlockInfo?.host}</S.SiteTitle>
        </S.SiteTitleBox>
      </S.HeaderBox>
      <S.Scroller>
        {isAllowedSectionVisible && (
        <div>
          <ScriptsInfo>
            <span>{props.resourcesList.allowedList.length}</span>
            <span>{props.totalAllowedTitle}</span>
            <span>{<a href="#" onClick={handleBlockAllScripts}>
                {getLocale('braveShieldsBlockScriptsAll')}
              </a>
            }</span>
          </ScriptsInfo>
          <ScriptsList>
            {[...allowedScriptsByOrigin.keys()].map((origin, idx) => {
              return (<TreeNode
                key={origin}
                host={origin}
                type={props.type}
                state={ResourceState.AllowedOnce}
                onPermissionButtonClick={handleBlockScript}
                permissionButtonTitle={getLocale('braveShieldsBlockScript')}
                resourceList={allowedScriptsByOrigin.get(origin) ?? []}
              />)
            })}
          </ScriptsList>
        </div>
        )}
        <ScriptsInfo>
          <span>{props.resourcesList.blockedList.length}</span>
          <span>{props.totalBlockedTitle}</span>
          {isAllowedSectionVisible && (<span>
            {<a href="#" onClick={handleAllowAllScripts}>
                {getLocale('braveShieldsAllowScriptsAll')}
              </a>
            }</span>
          )}
        </ScriptsInfo>
        <ScriptsList>
          {[...blockedScriptsByOrigin.keys()].map((origin, idx) => {
            return (<TreeNode
              key={idx}
              host={origin}
              state={ResourceState.Blocked}
              type={props.type}
              permissionButtonTitle={getLocale('braveShieldsAllowScriptOnce')}
              onPermissionButtonClick={permissionButtonHandler}
              resourceList={allowedScriptsByOrigin.get(origin) ?? []}
            />)
          })}
        </ScriptsList>
      </S.Scroller>
      <Footer>
        <Button
          aria-label="Back to previous screen"
          onClick={() => setViewType?.(ViewType.Main)}
        >
          <svg fill="currentColor" viewBox="0 0 32 32" aria-hidden="true"><path d="M28 15H6.28l4.85-5.25a1 1 0 0 0-.05-1.42 1 1 0 0 0-1.41.06l-6.4 6.93a.7.7 0 0 0-.1.16.75.75 0 0 0-.09.15 1 1 0 0 0 0 .74.75.75 0 0 0 .09.15.7.7 0 0 0 .1.16l6.4 6.93a1 1 0 0 0 1.41.06 1 1 0 0 0 .05-1.42L6.28 17H28a1 1 0 0 0 0-2z"/></svg>
          <span>{getLocale('braveShieldsStandalone')}</span>
        </Button>
      </Footer>
    </S.Box>
  )
}

export default TreeList
