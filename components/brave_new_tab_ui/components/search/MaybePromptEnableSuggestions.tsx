// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button from '@brave/leo/react/button';
import { useNewTabPref } from '../../hooks/usePref';
import * as React from 'react';
import styled from 'styled-components';
import { useSearchContext } from './SearchContext';
import { color, font, spacing } from '@brave/leo/tokens/css/variables';
import { getLocale } from '$web-common/locale';

const Container = styled.div`
    background: color-mix(in srgb, ${color.container.background} 10%, transparent 90%);
    color: color-mix(in srgb, ${color.text.primary} 70%, transparent 30%);

    padding: ${spacing['2Xl']};
    margin: -8px -8px 4px -8px;

    gap: ${spacing.l};
    display: flex;
    flex-direction: column;

    text-wrap: wrap;
`

const Title = styled.span`
    color: ${color.text.primary};
    font: ${font.default.semibold};
`

const Explanation = styled.span`
    font: ${font.small.regular};
`

const Actions = styled.div`
    display: flex;
    gap: ${spacing.s};
    justify-content: start;
    
    text-wrap: nowrap;

    > leo-button {
        flex: 0;
    }
`

export default function MaybePromptEnableSuggestions() {
    const [showPrompt, setShowPrompt] = useNewTabPref('promptEnableSearchSuggestions')
    const [suggestionsEnabled, setSuggestionsEnabled] = useNewTabPref('searchSuggestionsEnabled')
    const { query, setQuery } = useSearchContext()
    if (!showPrompt || suggestionsEnabled) return null
    return <Container>
        <Title>{getLocale('searchEnableSuggestionsPromptTitle')}</Title>
        <Explanation>
            {getLocale('searchEnableSuggestionsPromptExplanation')}
        </Explanation>
        <Actions>
            <Button onClick={() => {
                setShowPrompt(false)
                setSuggestionsEnabled(true)

                // Reset the query so we get suggestions
                setQuery('')
                setQuery(query)
            }}>{getLocale('searchEnableSuggestionsPromptEnable')}</Button>
            <Button onClick={() => setShowPrompt(false)} kind='plain-faint'>
                {getLocale('searchEnableSuggestionsPromptDismiss')}
            </Button>
        </Actions>
    </Container>
}
