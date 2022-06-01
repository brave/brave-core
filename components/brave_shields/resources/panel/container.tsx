import * as React from 'react'

import MainPanel from './components/main-panel'
import TreeList from './components/tree-list'
import { ViewType } from './state/component_types'
import DataContext from './state/context'
import styled from 'styled-components'
import { getLocale } from '../../../common/locale'

const Box = styled.div`
  position: relative;
`

function Container () {
  const { siteBlockInfo, viewType } = React.useContext(DataContext)
  const detailView = viewType !== ViewType.Main && siteBlockInfo

  const renderDetailView = () => {
    if (viewType === ViewType.AdsList && detailView) {
      return (<TreeList
        data={siteBlockInfo?.adsList}
        totalBlockedCount={siteBlockInfo?.adsList.length}
        blockedCountTitle={getLocale('braveShieldsTrackersAndAds')}
      />)
    }

    if (viewType === ViewType.HttpsList && detailView) {
      return (<TreeList
        data={siteBlockInfo?.httpRedirectsList}
        totalBlockedCount={siteBlockInfo?.httpRedirectsList.length}
        blockedCountTitle={getLocale('braveShieldsConnectionsUpgraded')}
      />)
    }

    if (viewType === ViewType.ScriptsList && detailView) {
      return (<TreeList
        data={siteBlockInfo?.jsList}
        totalBlockedCount={siteBlockInfo?.jsList.length}
        blockedCountTitle={getLocale('braveShieldsBlockedScriptsLabel')}
      />)
    }

    return null
  }

  return (
    <Box>
      {renderDetailView()}
      <MainPanel />
    </Box>
  )
}

export default Container
