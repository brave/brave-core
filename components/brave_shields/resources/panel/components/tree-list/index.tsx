import * as React from 'react'

import * as S from './style'
import DataContext from '../../state/context'
import { getLocale } from '../../../../../common/locale'
import TreeNode from './tree-node'
import { ViewType } from '../../state/component_types'
import { Url } from 'gen/url/mojom/url.mojom.m.js'
import Button from '$web-components/button'

interface Props {
  data: Url[]
  totalBlockedCount: number
  blockedCountTitle: string
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
  const mappedData = React.useMemo(() => groupByOrigin(props.data), [props.data])

  return (
    <S.Box>
      <S.HeaderBox>
        <S.SiteTitleBox>
          <S.FavIconBox>
            <img key={siteBlockInfo?.faviconUrl.url} src={siteBlockInfo?.faviconUrl.url} />
          </S.FavIconBox>
          <S.SiteTitle>{siteBlockInfo?.host}</S.SiteTitle>
        </S.SiteTitleBox>
        <S.Grid>
          <span>{props.totalBlockedCount}</span>
          <span>{props.blockedCountTitle}</span>
        </S.Grid>
      </S.HeaderBox>
      <S.TreeBox>
        <div>
          {[...mappedData.keys()].map((origin, idx) => {
            return (<TreeNode
              key={idx}
              host={origin}
              resourceList={mappedData.get(origin) ?? []}
            />)
          })}
        </div>
      </S.TreeBox>
      <S.Footer>
        <Button
          aria-label="Back to previous screen"
          onClick={() => setViewType?.(ViewType.Main)}
        >
          <svg fill="currentColor" viewBox="0 0 32 32" aria-hidden="true"><path d="M28 15H6.28l4.85-5.25a1 1 0 0 0-.05-1.42 1 1 0 0 0-1.41.06l-6.4 6.93a.7.7 0 0 0-.1.16.75.75 0 0 0-.09.15 1 1 0 0 0 0 .74.75.75 0 0 0 .09.15.7.7 0 0 0 .1.16l6.4 6.93a1 1 0 0 0 1.41.06 1 1 0 0 0 .05-1.42L6.28 17H28a1 1 0 0 0 0-2z"/></svg>
          <span>{getLocale('braveShields').replace('Brave ', '').trim()}</span>
        </Button>
      </S.Footer>
    </S.Box>
  )
}

export default TreeList
