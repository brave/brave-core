// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { ActionEntry, ActionType } from "components/ai_chat/resources/common/mojom";
import ToolsButtonMenu, { Props } from "./filter_menu";
import styles from "./style.module.scss";
import * as React from "react";
import { matches } from "./query";

function matchesQuery(query: string, entry: ActionEntry) {
  return entry.details && matches(query, entry.details.label)
}

export default function ToolsMenu({
  categories,
  handleClick,
  isOpen,
  setIsOpen,
  query,
}: {
  handleClick: (type: ActionType) => void
} & Pick<Props<ActionEntry>, "categories" | "isOpen" | "setIsOpen" | "query">) {
  return <ToolsButtonMenu
    categories={categories}
    isOpen={isOpen}
    setIsOpen={setIsOpen}
    query={query}
    matchesQuery={matchesQuery}
  >
    {(item) => !item.details
      ? <div className={styles.menuSubtitle}>{item.subheading}</div>
      : <leo-menu-item
        key={item.details!.type}
        onClick={() => handleClick(item.details!.type)}
      >
        {item.details.label}
      </leo-menu-item>}
  </ToolsButtonMenu>
}
