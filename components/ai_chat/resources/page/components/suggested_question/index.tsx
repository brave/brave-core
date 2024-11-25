// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from "@brave/leo/react/button";
import { useConversation } from "../../state/conversation_context";
import styles from './style.module.scss'

export function SuggestedQuestion({ question }: { question: string }) {
    const context = useConversation()
    return <SuggestionButton
        onClick={() => context.conversationHandler?.submitHumanConversationEntry(question)}
    >
        <span className={styles.buttonText}>{question}</span>
    </SuggestionButton>
}

export function SuggestionButton({ onClick, children, isLoading }: React.PropsWithChildren<{ onClick: () => void, isLoading?: boolean }>) {
    const context = useConversation()
    return <Button
        kind='outline'
        size='small'
        onClick={onClick}
        isDisabled={context.shouldDisableUserInput}
        isLoading={isLoading}
        className={styles.suggestion}>
        <div className={styles.container}>
            <div className={styles.icon}>
                💬
            </div>
            {children}
        </div>
    </Button>
}
