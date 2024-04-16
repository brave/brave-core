/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import org.chromium.chrome.browser.init.AsyncInitializationActivity
import org.chromium.mojo.bindings.ConnectionErrorHandler
import org.chromium.base.supplier.OneshotSupplier;
import org.chromium.mojo.system.MojoException
import org.chromium.playlist.mojom.PlaylistService
import org.chromium.chrome.browser.flags.ChromeFeatureList
import org.chromium.chrome.browser.init.ActivityProfileProvider
import org.chromium.base.BraveFeatureList
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences
import org.chromium.chrome.browser.profiles.ProfileProvider
import org.chromium.base.BravePreferenceKeys
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate

import android.view.View
import org.chromium.chrome.R

abstract  class PlaylistBaseActivity : AsyncInitializationActivity(), ConnectionErrorHandler, PlaylistServiceObserverImplDelegate {
    companion object {
        val TAG: String = "PlaylistBaseActivity"
    }
    protected var mPlaylistService: PlaylistService? = null
    protected var mPlaylistServiceObserver: PlaylistServiceObserverImpl? = null

    override fun onConnectionError(mojoException : MojoException) {
        mPlaylistService?.close()
        mPlaylistService = null
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)
                && ChromeSharedPreferences.getInstance()
                        .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)) {
            initPlaylistService()
        }
    }

    private fun initPlaylistService() {
        if (mPlaylistService != null) {
            mPlaylistService = null;
        }
        mPlaylistService =
                PlaylistServiceFactoryAndroid.getInstance()
                        .getPlaylistService(
                                getProfileProviderSupplier().get()?.getOriginalProfile(), this)
        addPlaylistObserver()
    }

    private fun addPlaylistObserver() {
        mPlaylistServiceObserver = PlaylistServiceObserverImpl(this)
        mPlaylistService?.addObserver(mPlaylistServiceObserver)
    }

    fun getPlaylistService() : PlaylistService? {
        return mPlaylistService
    }

    abstract fun initializeViews()

    override fun triggerLayoutInflation() {
        initializeViews()
        onInitialLayoutInflationComplete()
    }

    override fun finishNativeInitialization() {
        super.finishNativeInitialization()
    }
    
    override fun onResumeWithNative() {
        super.onResumeWithNative();
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            initPlaylistService();
        }
    }

    override fun onDestroy() {
            mPlaylistService?.close()
            mPlaylistService = null
            mPlaylistServiceObserver?.close()
            mPlaylistServiceObserver?.destroy()
            mPlaylistServiceObserver = null
        
        super.onDestroy();
    }

    override fun shouldStartGpuProcess() : Boolean {
        return true;
    }

    override fun createProfileProvider() : OneshotSupplier<ProfileProvider> {
        return ActivityProfileProvider(getLifecycleDispatcher());
    }
}
