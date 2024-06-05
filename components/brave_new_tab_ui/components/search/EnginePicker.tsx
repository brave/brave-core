// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from "$web-common/Flex";
import { getLocale } from "$web-common/locale";
import Button from '@brave/leo/react/button';
import ButtonMenu from '@brave/leo/react/buttonMenu';
import { color, spacing } from "@brave/leo/tokens/css/variables";
import * as React from "react";
import styled from "styled-components";
import { useSearchContext } from "./SearchContext";
import { MediumSearchEngineIcon } from "./SearchEngineIcon";

const Option = styled.div`
    display: flex;
    color: ${color.black};
    gap: ${spacing.m};
`

const CustomizeButton = styled(Button)`
  border-top: 1px solid ${color.divider.subtle};
  color: ${color.text.secondary};
`

const OpenButton = styled(Button)`
  margin: -6px 0 -6px ${spacing.m};
`

const IconContainer = styled(Flex)`
  margin-right: 16px;
`

export default function EnginePicker() {
  const { filteredSearchEngines, searchEngine, setSearchEngine, setOpen: setBoxOpen } = useSearchContext()
  const [open, setOpen] = React.useState(false)

  return <ButtonMenu data-theme="light" positionStrategy='fixed' isOpen={open} onClose={() => setOpen(false)}>
    <OpenButton fab kind="plain-faint" slot="anchor-content" onClick={() => setOpen(true)}>
      <IconContainer align="center" justify="center">
        <MediumSearchEngineIcon engine={searchEngine} />
      </IconContainer>
    </OpenButton>
    {filteredSearchEngines.map(s => <leo-menu-item onClick={() => setSearchEngine(s)} key={s.keyword}>
      <Option>
        <MediumSearchEngineIcon engine={s} />{s.name}
      </Option>
    </leo-menu-item>)}
    <CustomizeButton kind="plain-faint" size="small" onClick={() => {
      history.pushState(undefined, '', '?openSettings=Search')

      // For now, close the search box - the Settings dialog doesn't use a
      // dialog, so it gets rendered underneath.
      setBoxOpen(false)
    }}>
      {getLocale('searchCustomizeList')}
    </CustomizeButton>
  </ButtonMenu>
}
