// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import ProgressRing from '@brave/leo/react/progressRing'
import { getLocale } from '$web-common/locale'
import CaretSVG from '../svg/caret'
import cursorStyles from '../markdown_renderer/style.module.scss'
import styles from './style.module.scss'

interface QuoteProps {
  text: string
  isReasoning: boolean
}

function AssistantReasoning(props: QuoteProps) {
  const [showDetails, setShowDetails] = React.useState(false)

  return (
    <div className={styles.wrapper}>
      <div className={styles.reasoningRow}>
        {props.isReasoning ? (
          <ProgressRing className={styles.progressRing} />
        ) : (
          <Icon
            className={styles.icon}
            name='check-circle-outline'
          />
        )}
        <span className={styles.reasoningLabel}>
          {props.isReasoning
            ? getLocale(S.CHAT_UI_REASONING_LABEL)
            : getLocale(S.CHAT_UI_REASONING_COMPLETE_LABEL)}
        </span>
        <button
          className={styles.showHide}
          onClick={() => setShowDetails((prev) => !prev)}
        >
          {showDetails
            ? getLocale(S.CHAT_UI_HIDE_DETAILS_BUTTON_LABEL)
            : getLocale(S.CHAT_UI_SHOW_DETAILS_BUTTON_LABEL)}
        </button>
      </div>
      {showDetails && (
        <div className={styles.quote}>
          <p className={styles.text}>
            {props.text}
            {props.isReasoning && (
              <span className={cursorStyles.textCursor}>
                <CaretSVG />
              </span>
            )}
          </p>
        </div>
      )}
    </div>
  )
}

export default AssistantReasoning
