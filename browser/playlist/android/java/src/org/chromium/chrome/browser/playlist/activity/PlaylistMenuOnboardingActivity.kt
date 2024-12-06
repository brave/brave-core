/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.activity

import android.os.Bundle
import android.widget.ScrollView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatButton
import androidx.viewpager2.widget.ViewPager2
import org.chromium.chrome.browser.playlist.R
import org.chromium.chrome.browser.playlist.adapter.PlaylistOnboardingFragmentStateAdapter
import org.chromium.chrome.browser.playlist.extension.afterMeasured
import org.chromium.chrome.browser.playlist.extension.showOnboardingGradientBg
import org.chromium.chrome.browser.playlist.util.ConstantUtils
import org.chromium.chrome.browser.playlist.util.PlaylistUtils
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayoutMediator

class PlaylistMenuOnboardingActivity : AppCompatActivity(R.layout.playlist_onboarding_activity) {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val onboardingLayout = findViewById<ScrollView>(R.id.onboardingLayout)
        onboardingLayout.afterMeasured {
            showOnboardingGradientBg()
        }

        val playlistOnboardingViewPager: ViewPager2 = findViewById(R.id.playlistOnboardingViewPager)

        val playlistOnboardingFragmentStateAdapter = PlaylistOnboardingFragmentStateAdapter(
            this, PlaylistUtils.getOnboardingItemList(this)
        )
        playlistOnboardingViewPager.adapter = playlistOnboardingFragmentStateAdapter

        val nextButton: AppCompatButton = findViewById(R.id.btNextOnboarding)
        nextButton.setOnClickListener {
            if (playlistOnboardingViewPager.currentItem == 2) {
                PlaylistUtils.openBraveActivityWithUrl(this, ConstantUtils.PLAYLIST_FEATURE_YT_URL)
            } else {
                playlistOnboardingViewPager.currentItem =
                    playlistOnboardingViewPager.currentItem + 1
            }
        }

        playlistOnboardingViewPager.registerOnPageChangeCallback(object :
            ViewPager2.OnPageChangeCallback() {
            override fun onPageSelected(position: Int) {
                super.onPageSelected(position)
                nextButton.text =
                    if (position == 2) getString(R.string.playlist_try_it) else getString(R.string.playlist_next)
                playlistOnboardingFragmentStateAdapter.notifyItemChanged(position)
            }
        })

        val tabLayout: TabLayout = findViewById(R.id.playlistOnboardingTabLayout)
        TabLayoutMediator(tabLayout, playlistOnboardingViewPager) { tab, _ ->
            tab.setIcon(R.drawable.ic_tab_layout_dot_selector)
        }.attach()
    }
}
