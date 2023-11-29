import { getLocale } from "$web-common/locale"

export const getTranslatedChannelName = (channelName: string) => {
  try {
    return getLocale(`braveNewsChannel-${channelName}`)
  } catch (err) {
    console.error(`Couldn't find translation for channel '${channelName}'`)
    return channelName
  }
}
