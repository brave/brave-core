/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import android.os.Bundle
import android.widget.ScrollView
import androidx.appcompat.widget.AppCompatButton
import androidx.viewpager2.widget.ViewPager2

import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayoutMediator

import org.chromium.chrome.R
import org.chromium.chrome.browser.playlist.kotlin.adapter.PlaylistOnboardingFragmentStateAdapter
import org.chromium.chrome.browser.playlist.kotlin.extension.afterMeasured
import org.chromium.chrome.browser.playlist.kotlin.extension.showOnboardingGradientBg
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import org.chromium.chrome.browser.util.TabUtils

class PlaylistMenuOnboardingActivity : PlaylistBaseActivity() {

    override fun initializeViews() {
        setContentView(R.layout.activity_playlist_onboarding)
        val onboardingLayout = findViewById<ScrollView>(R.id.onboardingLayout)
        onboardingLayout.afterMeasured { showOnboardingGradientBg() }

        val playlistOnboardingViewPager: ViewPager2 = findViewById(R.id.playlistOnboardingViewPager)

        val playlistOnboardingFragmentStateAdapter =
            PlaylistOnboardingFragmentStateAdapter(this, PlaylistUtils.getOnboardingItemList(this))
        playlistOnboardingViewPager.adapter = playlistOnboardingFragmentStateAdapter

        val nextButton: AppCompatButton = findViewById(R.id.btNextOnboarding)
        nextButton.setOnClickListener {
            if (playlistOnboardingViewPager.currentItem == 2) {
                TabUtils.openURLWithBraveActivity(ConstantUtils.PLAYLIST_FEATURE_YT_URL)
            } else {
                playlistOnboardingViewPager.currentItem =
                    playlistOnboardingViewPager.currentItem + 1
            }
        }

        playlistOnboardingViewPager.registerOnPageChangeCallback(
            object : ViewPager2.OnPageChangeCallback() {
                override fun onPageSelected(position: Int) {
                    super.onPageSelected(position)
                    nextButton.text =
                        if (position == 2) getString(R.string.playlist_try_it)
                        else getString(R.string.playlist_next)
                    playlistOnboardingFragmentStateAdapter.notifyItemChanged(position)
                }
            }
        )

        val tabLayout: TabLayout = findViewById(R.id.playlistOnboardingTabLayout)
        TabLayoutMediator(tabLayout, playlistOnboardingViewPager) { tab, _ ->
                tab.setIcon(R.drawable.ic_tab_layout_dot_selector)
            }
            .attach()
    }
}
