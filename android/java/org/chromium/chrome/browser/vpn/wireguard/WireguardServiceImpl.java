/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.wireguard;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

import androidx.annotation.Nullable;

import com.wireguard.android.backend.Backend;
import com.wireguard.android.backend.BackendException;
import com.wireguard.android.backend.GoBackend;
import com.wireguard.android.backend.Tunnel;
import com.wireguard.config.Config;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.channels.BraveChannelDefinitions;
import org.chromium.chrome.browser.vpn.utils.BraveVpnUtils;

public class WireguardServiceImpl
        extends WireguardService.Impl implements TunnelModel.TunnelStateUpdateListener {
    private Backend mBackend;
    private TunnelModel mTunnelModel;
    private final IBinder mBinder = new LocalBinder();

    class LocalBinder extends Binder {
        WireguardServiceImpl getService() {
            return WireguardServiceImpl.this;
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public TunnelModel getTunnelModel() {
        return mTunnelModel;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mBackend = new GoBackend(ContextUtils.getApplicationContext());
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        new Thread() {
            @Override
            public void run() {
                try {
                    startVpn();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }.start();
        getService().startForeground(BraveChannelDefinitions.ChannelId.BRAVE_BROWSER_CHANNEL_INT,
                BraveVpnUtils.getBraveVpnNotification(ContextUtils.getApplicationContext()));
        return Service.START_NOT_STICKY;
    }

    private void startVpn() throws Exception {
        Config config = WireguardConfigUtils.loadConfig(ContextUtils.getApplicationContext());
        mTunnelModel = TunnelModel.createTunnel(config, this);
        mBackend.setState(mTunnelModel, Tunnel.State.UP, config);
    }

    @Override
    public void onDestroy() {
        try {
            mBackend.setState(mTunnelModel, Tunnel.State.DOWN, null);
        } catch (Exception e) {
            e.printStackTrace();
        }
        super.onDestroy();
    }

    @Override
    public void onTunnelStateUpdated(TunnelModel tunnelModel) {
        if (tunnelModel.getState() == Tunnel.State.DOWN) {
            getService().stopForeground(true);
            getService().stopSelf();
        }
    }
}
