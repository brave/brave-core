/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.fragment

import android.os.Bundle
import android.view.View
import androidx.appcompat.widget.AppCompatImageView
import androidx.appcompat.widget.AppCompatTextView
import androidx.fragment.app.Fragment
import org.chromium.chrome.browser.playlist.R
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistOnboardingModel
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils.ONBOARDING_MODEL

class PlaylistOnboardingSingleFragment : Fragment(R.layout.fragment_single_playlist_onboarding) {

    private var mPlaylistOnboardingModel: PlaylistOnboardingModel? = null

    @Suppress("deprecation")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let {
            mPlaylistOnboardingModel = it.getParcelable(ONBOARDING_MODEL)
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        val ivOnboardingIllustration: AppCompatImageView =
            view.findViewById(R.id.ivOnboardingIllustration)
        val tvOnboardingTitle: AppCompatTextView = view.findViewById(R.id.tvOnboardingTitle)
        val tvOnboardingMessage: AppCompatTextView = view.findViewById(R.id.tvOnboardingMessage)

        mPlaylistOnboardingModel?.let {
            tvOnboardingTitle.text = it.title
            tvOnboardingMessage.text = it.message
            ivOnboardingIllustration.setImageResource(it.illustration)
        }
    }

    companion object {
        @JvmStatic
        fun newInstance(model: PlaylistOnboardingModel) =
            PlaylistOnboardingSingleFragment().apply {
                arguments = Bundle().apply {
                    putParcelable(ONBOARDING_MODEL, model)
                }
            }
    }
}
