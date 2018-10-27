// /* This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
//  * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  Header,
  MainToggleFlex,
  Description,
  SiteInfoCard,
  DisabledTextGrid,
  Label,
  Highlight,
  UnHighlight,
  Toggle,
  ShieldIcon,
  MainSiteInfoGrid,
  ShieldIconFlex
} from 'brave-ui/features/shields'

// Types
import { Tab } from '../../types/state/shieldsPannelState'
import { BlockOptions } from '../../types/other/blockTypes'

// Utils
import * as shieldActions from '../../types/actions/shieldsPanelActions'
import { getMessage } from '../../background/api/localeAPI'
import { isHttpOrHttps } from '../../helpers/urlUtils'
import { getFavicon, getTotalResourcesBlocked } from '../../helpers/shieldsUtils'

export interface Props {
  tabData: Tab
  shieldsToggled: shieldActions.ShieldsToggled
}

export default class ShieldsHeader extends React.PureComponent<Props, {}> {
  onToggleShields = (event: React.ChangeEvent<HTMLInputElement>) => {
    const { origin } = this.props.tabData
    if (!isHttpOrHttps(origin)) {
      return
    }
    const shieldsOption: BlockOptions = event.target.checked ? 'allow' : 'block'
    this.props.shieldsToggled(shieldsOption)
  }

  renderEnabledContent = () => {
    return (
      <MainSiteInfoGrid>
        <Highlight enabled={true} size='large'>{getTotalResourcesBlocked(this.props.tabData)}</Highlight>
        <Label size='medium'>{getMessage('totalBlocked')}</Label>
      </MainSiteInfoGrid>
    )
  }

  renderDisabledContent = () => {
    return (
      <DisabledTextGrid>
        <ShieldIconFlex>
          <ShieldIcon />
        </ShieldIconFlex>
        <Description enabled={false}>{getMessage('disabledMessage')}</Description>
      </DisabledTextGrid>
    )
  }

  render () {
    const { braveShields, hostname, url } = this.props.tabData
    const enabled = braveShields !== 'block'

    return (
      <Header id='braveShieldsHeader' enabled={enabled}>
        <MainToggleFlex enabled={enabled}>
          <Label size='medium'>
            {getMessage('shields')} <Highlight enabled={enabled}> {enabled ? getMessage('up') : getMessage('down')}
            </Highlight>
            <UnHighlight> {getMessage('forThisSite')}</UnHighlight>
          </Label>
          <Toggle id='mainToggle' checked={enabled} onChange={this.onToggleShields} size='large' />
        </MainToggleFlex>
        {enabled ? <Description enabled={true}>{getMessage('enabledMessage')}</Description> : null}
        <SiteInfoCard>
          <MainSiteInfoGrid>
            <img src={getFavicon(url)} />
            <Label size='large'>{hostname}</Label>
          </MainSiteInfoGrid>
          {enabled ? this.renderEnabledContent() : this.renderDisabledContent()}
        </SiteInfoCard>
      </Header>
    )
  }
}
