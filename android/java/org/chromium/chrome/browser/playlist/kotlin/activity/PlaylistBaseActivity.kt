/*
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.activity

import org.chromium.base.BraveFeatureList
import org.chromium.base.BravePreferenceKeys
import org.chromium.base.Log
import org.chromium.base.supplier.OneshotSupplier
import org.chromium.chrome.browser.app.BraveActivity
import org.chromium.chrome.browser.flags.ChromeFeatureList
import org.chromium.chrome.browser.init.ActivityProfileProvider
import org.chromium.chrome.browser.init.AsyncInitializationActivity
import org.chromium.chrome.browser.playlist.PlaylistServiceFactoryAndroid
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl
import org.chromium.chrome.browser.playlist.PlaylistServiceObserverImpl.PlaylistServiceObserverImplDelegate
import org.chromium.chrome.browser.playlist.kotlin.enums.PlaylistOptionsEnum
import org.chromium.chrome.browser.playlist.kotlin.listener.PlaylistItemOptionsListener
import org.chromium.chrome.browser.playlist.kotlin.model.MoveOrCopyModel
import org.chromium.chrome.browser.playlist.kotlin.model.PlaylistItemOptionModel
import org.chromium.chrome.browser.playlist.kotlin.util.ConstantUtils
import org.chromium.chrome.browser.playlist.kotlin.util.PlaylistUtils
import org.chromium.chrome.browser.playlist.kotlin.view.bottomsheet.MoveOrCopyToPlaylistBottomSheet
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences
import org.chromium.chrome.browser.profiles.ProfileProvider
import org.chromium.chrome.browser.util.TabUtils
import org.chromium.mojo.bindings.ConnectionErrorHandler
import org.chromium.mojo.system.MojoException
import org.chromium.playlist.mojom.PlaylistItem
import org.chromium.playlist.mojom.PlaylistService

abstract class PlaylistBaseActivity :
    AsyncInitializationActivity(),
    ConnectionErrorHandler,
    PlaylistServiceObserverImplDelegate,
    PlaylistItemOptionsListener {
    companion object {
        val TAG: String = "PlaylistBaseActivity"
    }

    protected var mPlaylistId = ConstantUtils.DEFAULT_PLAYLIST
    protected var mPlaylistService: PlaylistService? = null
    protected var mPlaylistServiceObserver: PlaylistServiceObserverImpl? = null

    override fun onConnectionError(mojoException: MojoException) {
        mPlaylistService?.close()
        mPlaylistService = null
        if (
            ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST) &&
                ChromeSharedPreferences.getInstance()
                    .readBoolean(BravePreferenceKeys.PREF_ENABLE_PLAYLIST, true)
        ) {
            initPlaylistService()
        }
    }

    private fun initPlaylistService() {
        if (mPlaylistService != null) {
            mPlaylistService = null
        }
        mPlaylistService =
            PlaylistServiceFactoryAndroid.getInstance()
                .getPlaylistService(getProfileProviderSupplier().get()?.getOriginalProfile(), this)
        addPlaylistObserver()
    }

    private fun addPlaylistObserver() {
        mPlaylistServiceObserver = PlaylistServiceObserverImpl(this)
        mPlaylistService?.addObserver(mPlaylistServiceObserver)
    }

    fun getPlaylistService(): PlaylistService? {
        return mPlaylistService
    }

    open fun deletePlaylistItem(playlistItemOptionModel: PlaylistItemOptionModel) {}

    private fun openPlaylistInTab(isIncognito: Boolean, url: String) {
        try {
            val activity = BraveActivity.getBraveActivity()
            TabUtils.openUrlInNewTab(isIncognito, url)
            TabUtils.bringChromeTabbedActivityToTheTop(activity)
        } catch (e: BraveActivity.BraveActivityNotFoundException) {
            Log.e(TAG, "openPlaylistInTab error", e)
        }
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
        super.onResumeWithNative()
        if (ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_PLAYLIST)) {
            initPlaylistService()
        }
    }

    override fun onDestroy() {
        mPlaylistService?.close()
        mPlaylistService = null
        mPlaylistServiceObserver?.close()
        mPlaylistServiceObserver?.destroy()
        mPlaylistServiceObserver = null

        super.onDestroy()
    }

    override fun shouldStartGpuProcess(): Boolean {
        return true
    }

    override fun createProfileProvider(): OneshotSupplier<ProfileProvider> {
        return ActivityProfileProvider(getLifecycleDispatcher())
    }

    // PlaylistItemOptionsListener callback
    override fun onPlaylistItemOptionClicked(playlistItemOptionModel: PlaylistItemOptionModel) {
        when (playlistItemOptionModel.optionType) {
            PlaylistOptionsEnum.SHARE_PLAYLIST_ITEM -> {
                playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
                    PlaylistUtils.showSharingDialog(this@PlaylistBaseActivity, it)
                }
            }
            PlaylistOptionsEnum.OPEN_IN_NEW_TAB -> {
                playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
                    openPlaylistInTab(false, it)
                }
            }
            PlaylistOptionsEnum.OPEN_IN_PRIVATE_TAB -> {
                playlistItemOptionModel.playlistItem?.pageSource?.url?.let {
                    openPlaylistInTab(true, it)
                }
            }
            PlaylistOptionsEnum.DELETE_PLAYLIST_ITEM -> {
                deletePlaylistItem(playlistItemOptionModel)
            }
            PlaylistOptionsEnum.MOVE_PLAYLIST_ITEM,
            PlaylistOptionsEnum.COPY_PLAYLIST_ITEM -> {
                val moveOrCopyItems = ArrayList<PlaylistItem>()
                playlistItemOptionModel.playlistItem?.let { moveOrCopyItems.add(it) }
                PlaylistUtils.moveOrCopyModel =
                    MoveOrCopyModel(
                        playlistItemOptionModel.optionType,
                        mPlaylistId,
                        "",
                        moveOrCopyItems
                    )
                MoveOrCopyToPlaylistBottomSheet().show(supportFragmentManager, null)
            }
            else -> {
                // Do nothing
            }
        }
    }
}
