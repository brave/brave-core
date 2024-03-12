// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  SettingsContent,
  SettingsFeatureBody,
  SettingsMenu,
  SettingsSidebar,
  SettingsSidebarActiveButtonSlider,
  SettingsSidebarButton,
  SettingsSidebarButtonText,
  SettingsTitle,
  SettingsWrapper
} from '../../components/default'

import { BraveNewsContext } from '../../../brave_news/browser/resources/shared/Context'
import { Publishers } from '../../../brave_news/browser/resources/shared/api'
import { getLocale } from '$web-common/locale'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

// Tabs
const BackgroundImageSettings = React.lazy(() => import('./settings/backgroundImage'))
const BraveStatsSettings = React.lazy(() => import('./settings/braveStats'))
const TopSitesSettings = React.lazy(() => import('./settings/topSites'))
const ClockSettings = React.lazy(() => import('./settings/clock'))
const CardsSettings = React.lazy(() => import('./settings/cards'))

// Types
import { NewTabActions } from '../../constants/new_tab_types'

export interface Props {
  newTabData: NewTab.State
  actions: NewTabActions
  textDirection: string
  showSettingsMenu: boolean
  featureCustomBackgroundEnabled: boolean
  onClose: () => void
  onDisplayTodaySection: () => any
  onClearTodayPrefs: () => any
  toggleShowBackgroundImage: () => void
  toggleShowTopSites: () => void
  setMostVisitedSettings: (show: boolean, customize: boolean) => void
  toggleShowRewards: () => void
  toggleShowBraveTalk: () => void
  toggleBrandedWallpaperOptIn: () => void
  toggleCards: (show: boolean) => void
  chooseNewCustomImageBackground: () => void
  setCustomImageBackground: (selectedBackground: string) => void
  removeCustomImageBackground: (background: string) => void
  setBraveBackground: (selectedBackground: string) => void
  setColorBackground: (color: string, useRandomColor: boolean) => void
  onEnableRewards: () => void
  showBackgroundImage: boolean
  showTopSites: boolean
  customLinksEnabled: boolean
  brandedWallpaperOptIn: boolean
  allowBackgroundCustomization: boolean
  showRewards: boolean
  showBraveTalk: boolean
  braveRewardsSupported: boolean
  braveTalkSupported: boolean
  todayPublishers?: Publishers
  setActiveTab?: TabType
  cardsHidden: boolean
}

export enum TabType {
  BackgroundImage = 'backgroundImage',
  BraveStats = 'braveStats',
  TopSites = 'topSites',
  BraveNews = 'braveNews',
  Clock = 'clock',
  Cards = 'cards'
}

interface State {
  activeTab: TabType
}

const tabTypes = Object.values(TabType)

type TabMap<T> = { [P in TabType]: T }
const tabIcons: TabMap<string> = {
  [TabType.BackgroundImage]: 'image',
  [TabType.BraveNews]: 'product-brave-news',
  [TabType.BraveStats]: 'bar-chart',
  [TabType.Clock]: 'clock',
  [TabType.TopSites]: 'window-content',
  [TabType.Cards]: 'browser-ntp-widget',
}

const tabTranslationKeys: TabMap<string> = {
  [TabType.BackgroundImage]: 'backgroundImageTitle',
  [TabType.BraveNews]: 'braveNewsTitle',
  [TabType.BraveStats]: 'statsTitle',
  [TabType.Clock]: 'clockTitle',
  [TabType.TopSites]: 'topSitesTitle',
  [TabType.Cards]: 'cards',
}

export default class Settings extends React.PureComponent<Props, State> {
  static contextType: typeof BraveNewsContext = BraveNewsContext
  settingsMenuRef: React.RefObject<any>

  constructor(props: Props) {
    super(props)
    // Cache allowed tabs array on instance.
    // Feature flags won't change during page lifecycle, so we don't need to
    // change this when props change.
    this.settingsMenuRef = React.createRef()
    this.state = {
      activeTab: this.getInitialTab()
    }
  }

  handleClickOutside = (event: Event) => {
    if (
      this.settingsMenuRef &&
      this.settingsMenuRef.current &&
      !this.settingsMenuRef.current.contains(event.target) &&
      // Don't close the settings dialog for a click outside if we're in the
      // Brave News modal - the user expects closing that one to bring them back
      // to this one.
      !this.context.customizePage
    ) {
      this.props.onClose()
    }
  }

  componentDidMount() {
    document.addEventListener('mousedown', this.handleClickOutside)
    document.addEventListener('keydown', this.onKeyPressSettings)
  }

  componentWillUnmount() {
    document.removeEventListener('mousedown', this.handleClickOutside)
    document.removeEventListener('keydown', this.onKeyPressSettings)
  }

  componentDidUpdate(prevProps: Props) {
    if (prevProps.setActiveTab !== this.props.setActiveTab && this.props.setActiveTab) {
      this.setActiveTab(this.props.setActiveTab)
    }
    const isNewlyShown = (!prevProps.showSettingsMenu && this.props.showSettingsMenu)
    if (isNewlyShown) {
      this.setActiveTab(this.getInitialTab())
    }
  }

  onKeyPressSettings = (event: KeyboardEvent) => {
    if (event.key === 'Escape') {
      this.props.onClose()
    }
  }

  getInitialTab() {
    let tab = this.props.allowBackgroundCustomization
      ? TabType.BackgroundImage
      : TabType.BraveStats
    if (this.props.setActiveTab) {
      if (this.getActiveTabTypes().includes(this.props.setActiveTab)) {
        tab = this.props.setActiveTab
      }
    }
    return tab
  }

  toggleShowBackgroundImage = () => {
    this.props.toggleShowBackgroundImage()
  }

  setBraveBackground = (selectedBackground: string) => {
    this.props.setBraveBackground(selectedBackground)
  }

  setColorBackground = (color: string, useRandomColor: boolean) => {
    this.props.setColorBackground(color, useRandomColor)
  }

  setActiveTab(activeTab: TabType) {
    if (activeTab === TabType.BraveNews) {
      this.context.setCustomizePage('news')
      return
    }

    this.setState({ activeTab })
  }

  getActiveTabTypes = () => this.props.allowBackgroundCustomization
    ? tabTypes
    : tabTypes.filter(t => t !== TabType.BackgroundImage)

  render() {
    const {
      textDirection,
      showSettingsMenu,
      toggleShowTopSites,
      setMostVisitedSettings,
      toggleShowRewards,
      toggleShowBraveTalk,
      toggleBrandedWallpaperOptIn,
      showBackgroundImage,
      featureCustomBackgroundEnabled,
      showTopSites,
      customLinksEnabled,
      showRewards,
      showBraveTalk,
      brandedWallpaperOptIn,
      braveRewardsSupported,
      braveTalkSupported,
      toggleCards,
      cardsHidden,
      onEnableRewards
    } = this.props
    const { activeTab } = this.state

    if (!showSettingsMenu) {
      return null
    }

    const tabTypes = this.getActiveTabTypes()
    return (
      <SettingsWrapper textDirection={textDirection}>
        <SettingsMenu
          ref={this.settingsMenuRef}
          textDirection={textDirection}
          title={getLocale('dashboardSettingsTitle')}
        >
          <SettingsTitle id='settingsTitle'>
            <h1>{getLocale('dashboardSettingsTitle')}</h1>
            <Button fab kind='plain-faint' onClick={this.props.onClose}>
              <Icon name='close' />
            </Button>
          </SettingsTitle>
          <SettingsContent id='settingsBody'>
            <SettingsSidebar id='sidebar'>
              <SettingsSidebarActiveButtonSlider
                translateTo={tabTypes.indexOf(activeTab)}
              />
              {
                tabTypes.map((tabType) => {
                  const titleKey = tabTranslationKeys[tabType]
                  const isActive = activeTab === tabType
                  return (
                    <SettingsSidebarButton
                      tabIndex={0}
                      key={tabType}
                      data-active={isActive ? '' : null}
                      onClick={() => this.setActiveTab(tabType)}
                    >
                      <Icon name={tabIcons[tabType]} />
                      <SettingsSidebarButtonText
                        data-text={getLocale(titleKey)}>
                        {getLocale(titleKey)}
                      </SettingsSidebarButtonText>
                    </SettingsSidebarButton>
                  )
                })
              }
            </SettingsSidebar>
            <SettingsFeatureBody id='content'>
              {/* Empty loading fallback is ok here since we are loading from local disk. */}
              <React.Suspense fallback={(<div />)}>
                {activeTab === TabType.BackgroundImage && <BackgroundImageSettings
                  newTabData={this.props.newTabData}
                  toggleBrandedWallpaperOptIn={toggleBrandedWallpaperOptIn}
                  toggleShowBackgroundImage={this.toggleShowBackgroundImage}
                  chooseNewCustomImageBackground={this.props.chooseNewCustomImageBackground}
                  setCustomImageBackground={this.props.setCustomImageBackground}
                  removeCustomImageBackground={this.props.removeCustomImageBackground}
                  setBraveBackground={this.setBraveBackground}
                  setColorBackground={this.setColorBackground}
                  brandedWallpaperOptIn={brandedWallpaperOptIn}
                  showBackgroundImage={showBackgroundImage}
                  featureCustomBackgroundEnabled={featureCustomBackgroundEnabled}
                  onEnableRewards={onEnableRewards}
                  braveRewardsSupported={braveRewardsSupported}
                />}
                {activeTab === TabType.BraveStats && <BraveStatsSettings />}
                {activeTab === TabType.TopSites && <TopSitesSettings
                  toggleShowTopSites={toggleShowTopSites}
                  showTopSites={showTopSites}
                  customLinksEnabled={customLinksEnabled}
                  setMostVisitedSettings={setMostVisitedSettings}
                />}
                {activeTab === TabType.Clock && <ClockSettings />}
                {activeTab === TabType.Cards && <CardsSettings
                  toggleCards={toggleCards}
                  cardsHidden={cardsHidden}
                  toggleShowBraveTalk={toggleShowBraveTalk}
                  showBraveTalk={showBraveTalk}
                  braveTalkSupported={braveTalkSupported}
                  toggleShowRewards={toggleShowRewards}
                  braveRewardsSupported={braveRewardsSupported}
                  showRewards={showRewards}
                />}
              </React.Suspense>
            </SettingsFeatureBody>
          </SettingsContent>
        </SettingsMenu>
      </SettingsWrapper>
    )
  }
}
