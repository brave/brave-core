// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import getBraveNewsController, { BraveNewsInternals, Channel, Publisher, PublisherType, UserEnabled } from "./shared/api";

const internalsApi = BraveNewsInternals.getRemote()

export const getExportData = async () => {
    const api = getBraveNewsController()

    const { locale } = await api.getLocale()
    const { publishers } = await api.getPublishers();
    const { channels } = await api.getChannels()
    const channelsList: Channel[] = Object.values(channels)
    const subscribedChannels = channelsList
        .filter(c => c.subscribedLocales.some(c => c === locale))
        .map(c => c.channelName)

    const { suggestedPublisherIds } = await api.getSuggestedPublisherIds()
    const { urls: history } = await internalsApi.getVisitedSites()

    const publishersList: Publisher[] = Object.values(publishers)

    return `# locale
${locale}

# publishers
${publishersList
            .filter(p => p.type === PublisherType.DIRECT_SOURCE || p.userEnabledStatus !== UserEnabled.NOT_MODIFIED)
            .map((p) => `${p.publisherId}, ${p.publisherName}, ${p.feedSource.url}, ${p.userEnabledStatus}`)
            .join('\n')}

# channels
${subscribedChannels.join('\n')}

# suggested publishers
${suggestedPublisherIds.join('\n')}

# history
${history.join('\n')}
`
}

export const downloadExport = async () => {
    const result = await getExportData()

    const data = new Blob([result], { type: 'text/plain' })
    const url = URL.createObjectURL(data);

    const a = document.createElement('a');
    a.download = `brave-news-${new Date().toISOString()}.txt`
    a.href = url;

    document.body.appendChild(a)
    a.click();

    a.remove()

    URL.revokeObjectURL(url);
}
