/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

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

  return React.useMemo(() => {
    return {
      aiChat: {
        available: aiChatAvailable,
        visible: false,
        setVisible: () => {},
      },
      wallet: {
        available: walletAvailable,
        visible: false,
        setVisible: () => {},
      },
      rewards: {
        available: rewardsAvailable,
        visible: false,
        setVisible: () => {},
      },
      vpn: {
        available: vpnAvailable,
        visible: false,
        setVisible: () => {},
      },
    }
  }, [api, aiChatAvailable, walletAvailable, rewardsAvailable, vpnAvailable])
}

// Returns whether any feature can be displayed on the feature step.
export function hasFeaturesAvailable(features: ProductFeatures) {
  return Object.values(features).some((feature) => feature.available)
}
