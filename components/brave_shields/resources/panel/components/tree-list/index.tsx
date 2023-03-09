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
  ViewType
} from '../../state/component_types'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import Button from '$web-components/button'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import { ScriptsInfo, Footer, ScriptsList } from './style'


interface Props {
  blockedList: Url[]
  allowedList?: Url[]
  totalAllowedTitle?: string
  totalBlockedTitle: string
}

function groupByOrigin (data: Url[]) {
  const map: Map<string, string[]> = new Map()

  const includesDupeOrigin = (searchOrigin: string) => {
    const results = data.map(entry => new URL(entry.url).origin)
      .filter(entry => entry.includes(searchOrigin))
    return results.length > 1
  }

  data.forEach(entry => {
    const url = new URL(entry.url)
    const origin = url.origin
    const items = map.get(origin)

    if (items) {
      items.push(url.pathname + url.search)
      return // continue
    }

    // If the origin's full url is the resource itself then we show the full url as parent
    map.set(includesDupeOrigin(origin) ? origin : url.href.replace(/\/$/, ''), [])
  })

  return map
}

function TreeList (props: Props) {
  const { siteBlockInfo, setViewType } = React.useContext(DataContext)
  const allowedList = props.allowedList ?? [];
  const allowedScriptsByOrigin = React.useMemo(() =>
    groupByOrigin(allowedList), [allowedList])

  const blockedScriptsByOrigin = React.useMemo(() =>
    groupByOrigin(props.blockedList),
                  [props.blockedList])

  const handleAllowAllScripts = () => {
    const scripts = props.blockedList.map(entry => entry.url)
    getPanelBrowserAPI().dataHandler.allowScriptsOnce(scripts)
  }

  const handleBlockAllScripts = () => {
    const scripts = allowedList.map(entry => entry.url)
    getPanelBrowserAPI().dataHandler.blockAllowedScripts(scripts)
  }

  const handleBlockScript = (url: string) => {
    getPanelBrowserAPI().dataHandler.blockAllowedScripts([url])
  }

  const handleAllowScript = !props.allowedList ? undefined
    : (url: string) => {
      getPanelBrowserAPI().dataHandler.allowScriptsOnce([url])
    }

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
        {allowedList.length > 0 && (
        <>
          <ScriptsInfo>
            <span>{allowedList.length}</span>
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
                resourceList={allowedScriptsByOrigin.get(origin) ?? []}
                onPermissionButtonClick={handleBlockScript}
                permissionButtonTitle={getLocale('braveShieldsBlockScript')}
              />)
            })}
          </ScriptsList>
        </>
        )}
        <ScriptsInfo>
          <span>{props.blockedList.length}</span>
          <span>{props.totalBlockedTitle}</span>
          {props.allowedList && (<span>
            {<a href="#" onClick={handleAllowAllScripts}>
                {getLocale('braveShieldsAllowScriptsAll')}
              </a>
            }</span>
          )}
        </ScriptsInfo>
        <ScriptsList>
          {[...blockedScriptsByOrigin.keys()].map((origin, idx) => {
            return (<TreeNode
              key={origin}
              host={origin}
              resourceList={blockedScriptsByOrigin.get(origin) ?? []}
              onPermissionButtonClick={handleAllowScript}
              permissionButtonTitle={getLocale('braveShieldsAllowScriptOnce')}
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
