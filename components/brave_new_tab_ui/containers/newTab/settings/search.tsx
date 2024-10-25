// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import Flex from '$web-common/Flex'
import Button from '@brave/leo/react/button'
import Checkbox from '@brave/leo/react/checkbox'
import Icon from '@brave/leo/react/icon'
import Toggle from '@brave/leo/react/toggle'
import { color, font, radius, spacing } from '@brave/leo/tokens/css/variables'
import styled from 'styled-components'
import usePromise from '../../../../common/usePromise'
import {
  SettingsRow,
  SettingsText
} from '../../../components/default'
import { searchEnginesPromise } from '../../../components/search/SearchContext'
import { MediumSearchEngineIcon } from '../../../components/search/SearchEngineIcon'
import { useNewTabPref } from '../../../hooks/usePref'
import { getLocale } from '../../../../common/locale'
import { braveSearchHost } from '../../../components/search/config'
import { useEngineContext } from '../../../components/search/EngineContext'

const EnginesContainer = styled(Flex)`
  font: ${font.default.regular};
`

const EnginesGrid = styled.div`
  /* Three rows */
  max-height: 136px;
  overflow-y: auto;

  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: ${spacing.m};
`

const EngineCard = styled.div`
  border: 1px solid ${color.divider.subtle};
  border-radius: ${radius.s};

  padding: ${spacing.m};

  display: flex;
  gap: ${spacing.m};
`

const LinkButton = styled(Button)`
  --leo-button-color: ${color.text.secondary};
`

const EngineCheckbox = styled(Checkbox)`
  --leo-checkbox-flex-direction: row-reverse;

  width: 100%;
  font: ${font.default.regular};
`

const Hr = styled.hr`
  border-color: ${color.divider.subtle};
  border-bottom: unset;
  margin: 0;
`

const CheckboxText = styled.span`flex: 1`;
const hasEnabledEngine = (config: Record<string, boolean>) => Object.keys(config).some(key => config[key])

export default function SearchSettings() {
  const { result: engines = [] } = usePromise(() => searchEnginesPromise, [])
  const [showSearchBox, setShowSearchBox] = useNewTabPref('showSearchBox')
  const { setEngineConfig, engineConfig } = useEngineContext()

  React.useEffect(() => {
    if (!hasEnabledEngine(engineConfig)) {
      setShowSearchBox(false)
    }
  }, [engineConfig])

  return <Flex direction='column' gap={spacing.xl}>
    <SettingsRow>
      <SettingsText>{getLocale('searchShowSetting')}</SettingsText>
      <Toggle size='small' checked={showSearchBox} onChange={e => {
        setShowSearchBox(e.checked)

        // If we've just enabled the searchbox, make sure at least one engine
        // is enabled.
        if (e.checked && !hasEnabledEngine(engineConfig)) {
          setEngineConfig(braveSearchHost, true)
        }
      }} />
    </SettingsRow>
    {showSearchBox && <>
      <Hr />
      <EnginesContainer direction='column' gap={spacing.xl}>
        <div>{getLocale('searchEnableSearchEnginesTitle')}</div>
        <EnginesGrid>
          {engines.map(engine => <EngineCard key={engine.keyword}>
            <EngineCheckbox checked={engineConfig[engine.host]} onChange={e => {
              setEngineConfig(engine.host, e.checked)
            }}>
              <CheckboxText>{engine.name}</CheckboxText>
              <MediumSearchEngineIcon engine={engine} />
            </EngineCheckbox>
          </EngineCard>)}
        </EnginesGrid>
      </EnginesContainer>
      <Hr />
      <div>
        <LinkButton href='chrome://settings/searchEngines' kind='plain'>
          {getLocale('searchCustomizeSearchEngines')}
          <div slot='icon-after'><Icon name='launch' /></div>
        </LinkButton>
        <span />
      </div>
    </>}
  </Flex>
}
