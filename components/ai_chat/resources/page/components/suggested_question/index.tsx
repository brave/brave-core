// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from "@brave/leo/react/button";
import ProgressRing from '@brave/leo/react/progressRing';
import classnames from '$web-common/classnames';
import { getLocale } from '$web-common/locale';
import * as Mojom from '../../../common/mojom'
import { useConversation } from "../../state/conversation_context";
import styles from './style.module.scss'

export function SuggestedQuestion({ question }: { question: string }) {
    const context = useConversation()
    return <SuggestionButton
        onClick={() => context.conversationHandler?.submitHumanConversationEntry(question)}
        className={styles.questionButton}
    >
      <span className={styles.questionButtonText}>{question}</span>
    </SuggestionButton>
}

export function GenerateSuggestionsButton() {
  const conversationContext = useConversation()
  return (
    <SuggestionButton
      onClick={() => conversationContext.generateSuggestedQuestions()}
      isLoading={conversationContext.suggestionStatus === Mojom.SuggestionGenerationStatus.IsGenerating}
    >
      <span className={styles.generateButtonText}>
        {getLocale('suggestQuestionsLabel')}
      </span>
    </SuggestionButton>
  )
}

interface SuggestionButtonProps {
  onClick: () => void
  isLoading?: boolean
  className?: string
}

export function SuggestionButton(props: React.PropsWithChildren<SuggestionButtonProps>) {
    const context = useConversation()
    // TODO(petemill): Don't use Nala Button since our styles have
    // diverged and it's becoming difficult to position this content
    // full-width as well as use some features of the Button, like
    // the loading spinner.
    return <Button
        kind='outline'
        size='small'
        onClick={props.onClick}
        isDisabled={context.shouldDisableUserInput}
        isLoading={props.isLoading}
        className={classnames(styles.suggestion, props.className)}>
        <div className={styles.container}>
            <div className={styles.icon}>
                💬
            </div>
            <span className={styles.buttonText}>{props.children}</span>
            {props.isLoading && <ProgressRing />}
        </div>
    </Button>
}
