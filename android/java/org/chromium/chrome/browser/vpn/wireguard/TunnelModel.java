/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.wireguard;

import androidx.annotation.NonNull;

import com.wireguard.android.backend.Tunnel;
import com.wireguard.config.Config;

import org.chromium.base.Log;

public class TunnelModel implements Tunnel {
    public static final String TUNNEL_NAME = "Brave";
    private final String name;

    @SuppressWarnings("UnusedVariable")
    private Config config;

    private final TunnelStateUpdateListener tunnelStateUpdateListener;

    public State getState() {
        return state;
    }

    private Tunnel.State state;

    interface TunnelStateUpdateListener {
        void onTunnelStateUpdated(TunnelModel tunnelModel);
    }

    public static TunnelModel createTunnel(Config config,
            TunnelStateUpdateListener tunnelStateUpdateListener) throws IllegalAccessException {
        if (Tunnel.isNameInvalid(TUNNEL_NAME)) {
            throw new IllegalAccessException("Invalid name");
        }
        return new TunnelModel(TUNNEL_NAME, config, Tunnel.State.DOWN, tunnelStateUpdateListener);
    }

    private TunnelModel(String name, Config config, State state,
            TunnelStateUpdateListener tunnelStateUpdateListener) {
        this.name = name;
        this.config = config;
        this.state = state;
        this.tunnelStateUpdateListener = tunnelStateUpdateListener;
    }

    @NonNull
    @Override
    public String getName() {
        return name;
    }

    @Override
    public void onStateChange(@NonNull State newState) {
        this.state = newState;
        tunnelStateUpdateListener.onTunnelStateUpdated(this);
        Log.d("Tunnel State", newState.name());
    }
}
