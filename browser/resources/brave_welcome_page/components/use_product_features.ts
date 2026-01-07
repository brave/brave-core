/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Feature } from '../api/welcome_api'
import { useWelcomeApi } from '../api/welcome_api_context'

// A feature that can be displayed on the features step.
export interface ProductFeature {
  // Whether the product can be offered to the user.
  available: boolean

  // Whether any of the product's entry points, such as toolbar buttons and new
  // tab page cards, are visible.
  visible: boolean

  // Shows or hides all of the product's entry points.
  setVisible: (visible: boolean) => void
}

// The collection of product features that can be displayed on the features
// step.
export interface ProductFeatures {
  aiChat: ProductFeature
  wallet: ProductFeature
  rewards: ProductFeature
  vpn: ProductFeature
}

// Returns information about the features that can be shown to the user on the
// features step.
export function useProductFeatures(): ProductFeatures {
  const api = useWelcomeApi()

  const aiChatAvailable = api.useAiChatFeatureEnabledData()
  const walletAvailable = api.useWalletFeatureEnabledData()
  const rewardsAvailable = api.useRewardsFeatureEnabledData()
  const vpnAvailable = api.useVpnFeatureEnabledData()

  const visibility = api.useGetFeatureVisibilityData()

  return React.useMemo(() => {
    function visibilitySetter(feature: Feature) {
      return (visible: boolean) => {
        api.setFeatureVisible([feature, visible])
      }
    }
    return {
      aiChat: {
        available: aiChatAvailable,
        visible: visibility.aiChat,
        setVisible: visibilitySetter(Feature.kAIChat),
      },
      wallet: {
        available: walletAvailable,
        visible: visibility.wallet,
        setVisible: visibilitySetter(Feature.kWallet),
      },
      rewards: {
        available: rewardsAvailable,
        visible: visibility.rewards,
        setVisible: visibilitySetter(Feature.kRewards),
      },
      vpn: {
        available: vpnAvailable,
        visible: visibility.vpn,
        setVisible: visibilitySetter(Feature.kVPN),
      },
    }
  }, [
    api,
    aiChatAvailable,
    walletAvailable,
    rewardsAvailable,
    vpnAvailable,
    visibility,
  ])
}

// Returns whether any feature can be displayed on the feature step.
export function hasFeaturesAvailable(features: ProductFeatures) {
  return Object.values(features).some((feature) => feature.available)
}
