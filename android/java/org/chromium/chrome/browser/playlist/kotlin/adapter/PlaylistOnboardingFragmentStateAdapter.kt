/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.adapter

import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter
import org.chromium.chrome.browser.playlist.kotlin.fragment.PlaylistOnboardingSingleFragment
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOnboardingModel

class PlaylistOnboardingFragmentStateAdapter(
    fragmentActivity: FragmentActivity,
    private val onboardingPages: List<PlaylistOnboardingModel>
) :
    FragmentStateAdapter(fragmentActivity) {
    override fun getItemCount(): Int = onboardingPages.size

    override fun createFragment(position: Int): Fragment =
        PlaylistOnboardingSingleFragment.newInstance(onboardingPages[position])
}
