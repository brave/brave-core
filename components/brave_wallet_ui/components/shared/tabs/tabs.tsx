// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Styled Components
import {
  StyledWrapper,
  Tab,
  Indicator,
  TabWrapper,
  LabelSummary
} from './tabs.styles'

export interface TabOption {
  id: string
  label: string;
  labelSummary?: string | number;
}

export type NftTabOptionId = 'nfts' | 'hidden' | 'spam'

export interface NftTabOption extends TabOption {
  id: NftTabOptionId
}

interface Props {
  options: TabOption[]
  onSelect: (selected: TabOption) => void
}

export const Tabs = ({ options, onSelect }: Props) => {
  const [activeTab, setActiveTab] = React.useState<TabOption>(options[0])

  const onSelectTab = React.useCallback((option: TabOption) => {
    if(activeTab.id === option.id) return
    setActiveTab(option)
    onSelect(option)
  }, [activeTab, onSelect])

  return (
    <StyledWrapper>
      {options.map((option) => (
        <TabWrapper
          key={option.id}
          onClick={() => onSelectTab(option)}
        >
          <Tab
            isActive={option.id === activeTab.id}
          >
            {option.label}
            {option.labelSummary && (
              <LabelSummary isActive={activeTab.id === option.id}>
                {option.labelSummary}
              </LabelSummary>
            )}
          </Tab>
          <Indicator isActive={option.id === activeTab.id} />
        </TabWrapper>
      ))}
    </StyledWrapper>
  )
}
