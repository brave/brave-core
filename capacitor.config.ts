import type { CapacitorConfig } from '@capacitor/cli'

const config: CapacitorConfig = {
  appId: 'com.brave.ai',
  appName: 'Brave AI',
  webDir: 'components/ai_chat/resources/app/dist',
  ios: {
    path: 'components/ai_chat/resources/app-ios',
  },
  android: {
    path: 'components/ai_chat/resources/app-android',
  },
}

export default config
