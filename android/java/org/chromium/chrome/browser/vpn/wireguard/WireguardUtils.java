/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.wireguard;

import android.app.ActivityManager;
import android.content.Context;
import android.util.Pair;

import com.wireguard.config.BadConfigException;
import com.wireguard.config.Interface;
import com.wireguard.config.Peer;
import com.wireguard.crypto.KeyPair;

import java.util.ArrayList;
import java.util.List;

public class WireguardUtils {
    public static Interface getInterface(String address, String clientPrivateKey)
            throws BadConfigException {
        Interface.Builder builder = new Interface.Builder();
        builder.parseAddresses(address);
        builder.parseDnsServers("1.1.1.1, 1.0.0.1");
        builder.parseListenPort("51821");
        builder.parsePrivateKey(clientPrivateKey);
        return builder.build();
    }

    public static List<Peer> getPeers(String host, String serverPublicKey)
            throws BadConfigException {
        List<Peer> peers = new ArrayList<>();
        Peer.Builder builder = new Peer.Builder();
        builder.parseAllowedIPs("0.0.0.0/0");
        builder.parseEndpoint(host + ":51821");
        builder.parsePublicKey(serverPublicKey);
        peers.add(builder.build());
        return peers;
    }

    public static boolean isServiceRunningInForeground(Context context, Class<?> serviceClass) {
        ActivityManager manager =
                (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service :
                manager.getRunningServices(Integer.MAX_VALUE)) {
            if (serviceClass.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
    }
}
