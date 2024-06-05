// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Flex from "$web-common/Flex";
import { getLocale } from "$web-common/locale";
import Button from '@brave/leo/react/button';
import ButtonMenu from '@brave/leo/react/buttonMenu';
import Floating from '@brave/leo/react/floating';
import { color, radius, spacing } from "@brave/leo/tokens/css/variables";
import * as React from "react";
import styled, { css } from "styled-components";
import { useSearchContext } from "./SearchContext";
import { MediumSearchEngineIcon } from "./SearchEngineIcon";

const Option = styled.div`
    display: flex;
    gap: ${spacing.m};

    padding: ${spacing.m};
    border-radius: ${radius.s};

    &:hover, &[data-selected=true] {
      background: rgba(255,255,255,0.1);
    }
`

const CustomizeButton = styled(Button)`
  border-top: 1px solid ${color.divider.subtle};
  color: ${color.text.secondary};
`

const OpenButton = styled(Button)`
  margin: -6px 0 -6px ${spacing.s};
`

const IconContainer = styled(Flex) <{ open: boolean }>`
  margin-right: 4px;
  border-radius: ${radius.s};
  padding: ${spacing.s};

  width: 32px;
  height: 32px;

  ${p => p.open && css`
    background: rgba(255,255,255,0.25);
  `}
`

const Menu = styled.div`
  width: 180px;

  background: rgba(255,255,255,0.1);
  backdrop-filter: blur(64px);

  border-radius: ${radius.m};
  padding: ${spacing.s};

  display: flex;
  flex-direction: column;
  gap: ${spacing.s};
`

const findParentWithTag = (el: HTMLElement | null | undefined, tagName: string) => {
  do {
    el = el?.parentElement
  } while (el && el.tagName !== tagName)

  return el;
}

export default function EnginePicker() {
  const { filteredSearchEngines, searchEngine, setSearchEngine, setOpen: setBoxOpen } = useSearchContext()
  const [open, setOpen] = React.useState(false)

  const ref = React.useRef<HTMLElement>();


  return <>
    <OpenButton ref={ref} fab kind="plain-faint" slot="anchor-content" onClick={() => setOpen(o => !o)}>
      <IconContainer align="center" justify="center" open={open}>
        <MediumSearchEngineIcon engine={searchEngine} />
      </IconContainer>
    </OpenButton>
    {open && <Floating target={findParentWithTag(ref.current, 'LEO-INPUT')!} autoUpdate positionStrategy="fixed" placement="top-start" >
      <Menu data-theme="dark">
        {filteredSearchEngines.map(s => <Option onClick={() => {
          setSearchEngine(s)
          setOpen(false)
        }}
          key={s.keyword}
          data-selected={s === searchEngine}>
          <MediumSearchEngineIcon engine={s} />{s.name}
        </Option>)}
        <CustomizeButton kind="plain-faint" size="small" onClick={() => {
          history.pushState(undefined, '', '?openSettings=Search')

          // For now, close the search box - the Settings dialog doesn't use a
          // dialog, so it gets rendered underneath.
          setBoxOpen(false)
        }}>
          {getLocale('searchCustomizeList')}
        </CustomizeButton>
      </Menu>
    </Floating>}
  </>
  return <ButtonMenu data-theme="light" positionStrategy='fixed' isOpen={open} onClose={() => setOpen(false)}>

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
