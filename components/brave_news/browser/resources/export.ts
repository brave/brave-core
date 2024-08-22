import getBraveNewsController, { BraveNewsInternals, Channel, Publisher, PublisherType, UserEnabled } from "./shared/api";

const internalsApi = BraveNewsInternals.getRemote()

export const getExportData = async () => {
    const api = getBraveNewsController()

    const { locale } = await api.getLocale()
    const { publishers } = await api.getPublishers();
    const { channels } = await api.getChannels()
    const subscribedChannels = (Object.values(channels) as Channel[])
        .filter(c => c.subscribedLocales.some(c => c === locale))
        .map(c => c.channelName)

    const { suggestedPublisherIds } = await api.getSuggestedPublisherIds()
    const { urls: history } = await internalsApi.getVisitedSites()

    return `# locale
${locale}

# publishers
${(Object.values(publishers) as Publisher[])
            .filter(p => p.type == PublisherType.DIRECT_SOURCE || p.userEnabledStatus !== UserEnabled.NOT_MODIFIED)
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
