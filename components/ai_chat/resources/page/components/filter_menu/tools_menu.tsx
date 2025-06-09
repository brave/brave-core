// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from "react";

import { ActionEntry, ActionType } from "components/ai_chat/resources/common/mojom";
import FilterMenu, { Props } from "./filter_menu";
import { matches } from "./query";
import styles from "./style.module.scss";

type ToolsMenuProps = {
  handleClick: (type: ActionType) => void
} & Pick<Props<ActionEntry>, "categories" | "isOpen" | "setIsOpen" | "query">

function matchesQuery(query: string, entry: ActionEntry) {
  return entry.details && matches(query, entry.details.label)
}

export default function ToolsMenu(props: ToolsMenuProps) {
  return <FilterMenu
    categories={props.categories}
    isOpen={props.isOpen}
    setIsOpen={props.setIsOpen}
    query={props.query}
    matchesQuery={matchesQuery}
  >
    {(item) => !item.details
      ? <div className={styles.menuSubtitle}>{item.subheading}</div>
      : <leo-menu-item
        key={item.details.type}
        onClick={() => props.handleClick(item.details!.type)}
      >
        {item.details.label}
      </leo-menu-item>}
  </FilterMenu>
}
