/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific Components
import {
  Page,
  PageWrapper,
  PageHeader,
  StatsContainer,
  StatsItem,
  FeatureBlock,
  NewTabHeading,
  SmallText,
  EmphasizedText,
  LabelledText,
  Paragraph,
  LinkText
} from '../../../../src/features/newTab'

// Components
import Clock from '../../../../src/features/newTab/clock'
import Toggle from '../../../../src/components/formControls/toggle'

// Assets
import '../../../assets/fonts/muli.css'
import '../../../assets/fonts/poppins.css'
import locale from './fakeLocale'
import data from './fakeData'

// Images
const ddgIcon = require('../../../assets/img/private_tab_pagearea_ddgicon.svg')
const torIcon = require('../../../assets/img/toricon.svg')
const torFAQ = 'https://github.com/brave/browser-laptop/wiki/Using-Tor-in-Brave#faq'

interface Props {
  isTor: boolean
}

export default class NewPrivateTab extends React.PureComponent<Props, {}> {
  render () {
    const isTor = this.props.isTor
    return (
      <Page>
        <PageWrapper>
          <PageHeader>
            <StatsContainer testId='stats'>
              <StatsItem
                counter={data.trackersBlockedCount}
                description={locale.trackersBlocked}
              />
              <StatsItem
                counter={data.adsBlockedCount}
                description={locale.adsBlocked}
              />
              <StatsItem
                counter={data.httpsUpgradesCount}
                description={locale.httpsUpgrades}
              />
              <StatsItem
                counter={data.estimatedTimeCount}
                text={locale.minutes}
                description={locale.estimatedTime}
              />
            </StatsContainer>
            <Clock />
          </PageHeader>
          <div>
            <NewTabHeading>
              {
                isTor ? locale.thisIsAPrivateWindowWithTor : locale.thisIsAPrivateWindow
              }
            </NewTabHeading>
            {
              !isTor
              ? (
                <FeatureBlock grid={true}>
                  <aside>
                    <img src={torIcon} />
                  </aside>
                  <article>
                    <NewTabHeading level={2}>
                      {locale.makeThisTabMuchMorePrivateWith}&nbsp;
                      <EmphasizedText>{locale.tor}</EmphasizedText>
                      <LabelledText>{locale.beta}</LabelledText>
                    </NewTabHeading>
                    <Paragraph>{locale.torDisclaimer}</Paragraph>
                    <LinkText href={torFAQ} target='_blank' rel='noreferrer noopener'>
                    {locale.learnMore}
                    </LinkText>
                  </article>
                </FeatureBlock>
              )
            : null
            }
            {
              isTor
              ? (
                <FeatureBlock>
                  <Paragraph>
                    {locale.defaultSearchEngineDisclaimer}&nbsp;&nbsp;
                    <a href='#' target='blank' rel='noreferrer noopener'>
                      {locale.searchPreferences}
                    </a>
                  </Paragraph>
                </FeatureBlock>
              )
              : (
                <FeatureBlock grid={true}>
                  <aside>
                    <img src={ddgIcon} />
                  </aside>
                  <article>
                    <NewTabHeading level={2}>
                      {locale.privateSearchWith}&nbsp;
                      <EmphasizedText>{locale.duckduckgo}</EmphasizedText>
                    </NewTabHeading>
                      <Paragraph>{locale.duckduckgoDisclaimer}</Paragraph>
                  </article>
                  <aside>
                    <Toggle checked={true} />
                  </aside>
                </FeatureBlock>
              )
            }
            <FeatureBlock>
              <NewTabHeading level={3}>{locale.moreAboutPrivateTabs}</NewTabHeading>
              <SmallText>{locale.privateTabsDisclaimer}</SmallText>
            </FeatureBlock>
          </div>
        </PageWrapper>
      </Page>
    )
  }
}
