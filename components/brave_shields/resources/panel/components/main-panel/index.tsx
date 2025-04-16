// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

import * as S from './style'
import Toggle from '../../../../../web-components/toggle'
import AdvancedControlsContent from '../advanced-controls-content'
import AdvancedControlsContentScroller from '../advanced-controls-scroller'
import { formatLocale, getLocale } from '$web-common/locale'
import DataContext from '../../state/context'
import getPanelBrowserAPI from '../../api/panel_browser_api'
import Button from '$web-components/button'
import { useIsExpanded } from '../../state/hooks'

const handleLearnMoreClick = () => {
  chrome.tabs.create({ url: 'https://brave.com/privacy-features/', active: true })
}

function MainPanel () {
  const { isExpanded, toggleIsExpanded } = useIsExpanded()
  const { siteBlockInfo, getSiteSettings } = React.useContext(DataContext)

  const braveShieldsStatus = formatLocale(siteBlockInfo?.isBraveShieldsEnabled
    ? 'braveShieldsUp'
    : 'braveShieldsDown', {
        $1: (content) => <span>{content}</span>,
        $2: () => siteBlockInfo?.host
    })

  const braveShieldsBrokenText = formatLocale('braveShieldsBroken', {
    $1: content => <span>{content}</span>
  })

  const braveShieldsNote = formatLocale(siteBlockInfo?.isBraveShieldsEnabled
    ? 'braveShieldsBlockedNote'
    : 'braveShieldsNOTBlockedNote', {
      $1: content => <a href="#" onClick={handleLearnMoreClick}>{content}</a>
    })

  const handleToggleChange = async (isOn: boolean) => {
    await getPanelBrowserAPI().dataHandler.setBraveShieldsEnabled(isOn)
    if (isOn) {
      if (getSiteSettings) getSiteSettings()
    }
  }

  const handleReportSite = async () => {
    await getPanelBrowserAPI().dataHandler.openWebCompatWindow()
  }

  const [areAnyBlockedElementsPresent,
    setAreAnyBlockedElementsPresent] = React.useState(false);
  React.useEffect(() => {
    const getBlockedElementsAvailability = () => {
       getPanelBrowserAPI().dataHandler.areAnyBlockedElementsPresent()
        .then((data) => {
          setAreAnyBlockedElementsPresent(data.isAvailable)
        })
    };

    document.addEventListener('visibilitychange',
      getBlockedElementsAvailability)
    getBlockedElementsAvailability()

    return () => {
      document.removeEventListener('visibilitychange',
        getBlockedElementsAvailability)
    }
  }, []);

  const handleResetBlockedElements = async () => {
    await getPanelBrowserAPI().dataHandler.resetBlockedElements()
    setAreAnyBlockedElementsPresent(false)
  }

  const onSettingsClick = () => {
    chrome.tabs.create({ url: 'chrome://settings/shields', active: true })
  }

  let reportSiteOrFootnoteElement = (
    <S.Footnote>
      {braveShieldsBrokenText}
    </S.Footnote>
  )
  let managedFootnoteElement = (
    <S.Footnote>
      <S.ControlBox>
        <S.ManagedIcon />
        <S.ManagedText>{getLocale('braveShieldsManaged')}</S.ManagedText>
      </S.ControlBox>
    </S.Footnote>
  )
  let advancedControlButtonElement = (isExpanded != null) && (
    <S.AdvancedControlsButton
      type="button"
      aria-expanded={isExpanded}
      aria-controls='advanced-controls-content'
      onClick={toggleIsExpanded}
    >
      <i>
        <svg width="16" height="16" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M15.334 8.969H6a.667.667 0 1 1 0-1.334h9.334a.667.667 0 0 1 0 1.334Zm.005-5.377H5.962c-.368 0-.629-.255-.629-.623s.299-.667.667-.667h9.334c.367 0 .666.299.666.667 0 .368-.292.623-.66.623ZM2 15.635c-1.102 0-2-.897-2-2s.898-2 2-2c1.103 0 2 .897 2 2s-.897 2-2 2Zm0-2.666a.667.667 0 1 0 .001 1.334.667.667 0 0 0 0-1.334Zm0-2.667c-1.102 0-2-.897-2-2s.898-2 2-2c1.103 0 2 .897 2 2s-.897 2-2 2Zm0-2.667a.667.667 0 1 0 .002 1.335A.667.667 0 0 0 2 7.635Zm.398-3.604a.669.669 0 0 1-.96.12L.244 3.17a.666.666 0 1 1 .846-1.03l.65.533L2.798 1.24a.668.668 0 0 1 1.073.791l-1.472 2ZM6 12.969h9.334a.667.667 0 0 1 0 1.333H6a.667.667 0 1 1 0-1.333Z"/></svg>
      </i>
      <span>{getLocale('braveShieldsAdvancedCtrls')}</span>
      <S.CaratIcon isExpanded={isExpanded} />
    </S.AdvancedControlsButton>
  )

  let totalCountElement = (
    <S.BlockCount title={siteBlockInfo?.totalBlockedResources.toString()}>
      {(siteBlockInfo?.totalBlockedResources ?? 0) > 99 ? '99+' : siteBlockInfo?.totalBlockedResources}
    </S.BlockCount>
  )

  if (!siteBlockInfo?.isBraveShieldsEnabled) {
    totalCountElement = (<S.BlockCount>{'\u2014'}</S.BlockCount>)

    advancedControlButtonElement = (
      <S.GlobalDefaultsButton
        type="button"
        onClick={onSettingsClick}
      >
        <i className="icon-globe">
          <svg width="18" height="18" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M14.03 11.126a3.191 3.191 0 0 1 3.188 3.186A3.191 3.191 0 0 1 14.03 17.5a3.191 3.191 0 0 1-3.187-3.188 3.191 3.191 0 0 1 3.187-3.186Zm0 1.417c-.227 0-.443.046-.643.125l2.289 2.288c.078-.2.125-.416.125-.644 0-.976-.795-1.77-1.77-1.77Zm0 3.54c.228 0 .444-.046.644-.125l-2.29-2.29c-.078.2-.125.417-.125.644 0 .977.795 1.772 1.771 1.772Z"/><path d="M8.718.5C4.043.5.218 4.325.218 9s3.825 8.5 8.5 8.5h.354a.71.71 0 0 0 .708-.708c0-.355-.354-.638-.708-.638-.708-.92-1.558-2.267-2.054-3.966H8.93c.425 0 .709-.284.709-.709s-.284-.708-.709-.708H6.734c0-.142-.07-.284-.07-.425-.142-1.133-.071-2.054.07-2.975h3.896c.071.637.142 1.346.142 2.054 0 .425.283.708.708.708a.71.71 0 0 0 .709-.708c0-.708 0-1.417-.142-2.125h3.542c.141.496.212 1.063.212 1.63 0 .283 0 .566-.07.85-.072.353.212.708.637.778.354.071.708-.212.779-.637.07-.213.07-.567.07-.921 0-4.675-3.824-8.5-8.5-8.5ZM7.23 2.058c-.566.992-1.275 2.267-1.629 3.896H2.343c.92-1.983 2.762-3.4 4.887-3.896Zm0 13.884c-2.125-.496-3.896-1.913-4.816-3.825H5.6a11.962 11.962 0 0 0 1.63 3.825Zm-1.983-5.525c0 .07 0 .212.07.283h-3.47c-.142-.496-.213-1.133-.213-1.7s.071-1.133.213-1.63h3.471c-.142.922-.213 1.984-.071 3.047Zm3.33-4.463H7.088a12.229 12.229 0 0 1 1.629-3.47c.566.85 1.204 1.983 1.629 3.47h-1.77Zm6.516 0h-3.33c-.424-1.629-1.062-2.975-1.7-3.896 2.196.425 4.109 1.913 5.03 3.896Z"/></svg>
        </i>
        <span>{getLocale('braveShieldsChangeDefaults')}</span>
      </S.GlobalDefaultsButton>
    )

    reportSiteOrFootnoteElement = (
      <S.ReportSiteBox>
        <S.ReportSiteAction>
          <span>{getLocale('braveShieldsReportSiteDesc')}</span>
          <Button
            isPrimary
            onClick={handleReportSite}
          >
            {getLocale('braveShieldsReportSite')}
          </Button>
        </S.ReportSiteAction>
      </S.ReportSiteBox>
    )
  }

  return (
    <S.Box>
      <S.HeaderBox>
      <S.SiteTitleBox>
        <S.FavIconBox>
          <img key={siteBlockInfo?.faviconUrl.url} src={siteBlockInfo?.faviconUrl.url} />
        </S.FavIconBox>
        <S.SiteTitle>{siteBlockInfo?.host}</S.SiteTitle>
      </S.SiteTitleBox>
      <S.CountBox>
        <S.BlockNote>
          {braveShieldsNote}
        </S.BlockNote>
        {totalCountElement}
      </S.CountBox>
      </S.HeaderBox>
      <S.StatusBox>
        <S.ControlBox>
          <S.ShieldsIcon isActive={siteBlockInfo?.isBraveShieldsEnabled ?? false}>
            <svg width="24" height="28" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fillRule="evenodd" clipRule="evenodd" d="M23.654 11.667c-.002.122-.258 12.26-11.255 16.263-.008.002-.017 0-.024.003A1.16 1.16 0 0 1 12 28c-.13 0-.256-.026-.376-.067l-.023-.003C.604 23.927.347 11.789.346 11.667.339 11.02.333 9.965.333 9.32V5.817a2.338 2.338 0 0 1 2.333-2.335h1.167c4.9 0 7.106-2.9 7.197-3.023.222-.298.558-.443.902-.455.394-.032.794.117 1.04.46.088.118 2.294 3.018 7.194 3.018h1.167a2.338 2.338 0 0 1 2.333 2.335V9.32c0 .645-.006 1.701-.012 2.347Zm-2.321-5.85h-1.167c-4.263 0-6.895-1.827-8.166-3.009-1.272 1.182-3.904 3.01-8.167 3.01H2.666V9.32c0 .639.006 1.684.013 2.322.006.425.272 10.332 9.32 13.92 9.085-3.609 9.32-13.815 9.321-13.92.007-.638.013-1.683.013-2.322V5.817Zm-8.882 12.368a1.167 1.167 0 0 1-1.688.203l-4.177-3.503a1.169 1.169 0 0 1-.145-1.646 1.165 1.165 0 0 1 1.643-.144L11.31 15.8l4.42-6.004a1.166 1.166 0 1 1 1.879 1.385l-5.157 7.005Z" />
            </svg>
          </S.ShieldsIcon>
          <S.StatusText>
            {braveShieldsStatus}
          </S.StatusText>
          <S.StatusToggle>
            <Toggle
              brand="shields"
              isOn={siteBlockInfo?.isBraveShieldsEnabled}
              onChange={handleToggleChange}
              accessibleLabel={getLocale('braveShieldsEnable')}
              disabled={siteBlockInfo?.isBraveShieldsManaged}
            />
          </S.StatusToggle>
        </S.ControlBox>
        {!siteBlockInfo?.isBraveShieldsManaged &&
        <S.StatusFootnoteBox>
          {reportSiteOrFootnoteElement}
        </S.StatusFootnoteBox>
        }
        {siteBlockInfo?.isBraveShieldsManaged &&
        <S.StatusFootnoteBox>
          {managedFootnoteElement}
        </S.StatusFootnoteBox>
        }
      </S.StatusBox>
      {advancedControlButtonElement}
      { isExpanded &&
        siteBlockInfo?.isBraveShieldsEnabled &&
        <AdvancedControlsContentScroller
          isExpanded={isExpanded}
        >
          <AdvancedControlsContent />
        </AdvancedControlsContentScroller>
      }
      {
        areAnyBlockedElementsPresent &&
        <S.GlobalDefaultsButton
          type="button"
          onClick={handleResetBlockedElements}
        >
          <span>{getLocale('braveShieldsShowAllBlockedElems')}</span>
        </S.GlobalDefaultsButton>
      }
    </S.Box>
  )
}

export default MainPanel
