/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.vpn.wireguard;

import android.app.ActivityManager;
import android.content.Context;

import com.wireguard.config.BadConfigException;
import com.wireguard.config.Interface;
import com.wireguard.config.Peer;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.vpn.utils.BraveVpnPrefUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class WireguardUtils {
    public static Interface getInterface(String address, String clientPrivateKey)
            throws BadConfigException {
        Interface.Builder builder = new Interface.Builder();
        builder.parseAddresses(address);
        builder.parseDnsServers("1.1.1.1, 1.0.0.1");
        builder.parseListenPort("51821");
        builder.parsePrivateKey(clientPrivateKey);
        builder.excludeApplications(BraveVpnPrefUtils.getExcludedPackages());
        return builder.build();
    }

    public static Interface getInterface(Interface existingInterface) throws BadConfigException {
        Interface.Builder builder = new Interface.Builder();
        builder.addAddresses(existingInterface.getAddresses());
        builder.parseDnsServers("1.1.1.1, 1.0.0.1");
        builder.parseListenPort("51821");
        builder.parsePrivateKey(existingInterface.getKeyPair().getPrivateKey().toBase64());
        builder.excludeApplications(BraveVpnPrefUtils.getExcludedPackages());
        return builder.build();
    }

    public static List<Peer> getPeers(
            String host, String serverPublicKey, boolean excludeLANTraffic)
            throws BadConfigException {
        String allowedIPs = "0.0.0.0/0, ::/0";
        if (excludeLANTraffic) {
            allowedIPs =
                    "0.0.0.0/5,8.0.0.0/7,11.0.0.0/8,12.0.0.0/6,16.0.0.0/4,32.0.0.0/3,64.0.0.0/2,128.0.0.0/3,"
                        + "160.0.0.0/5,168.0.0.0/6,172.0.0.0/12,172.32.0.0/11,172.64.0.0/10,172.128.0.0/9,173.0.0.0/8,"
                        + "174.0.0.0/7,176.0.0.0/4,192.0.0.0/9,192.128.0.0/11,192.160.0.0/13,192.169.0.0/16,192.170.0.0/15,"
                        + "192.172.0.0/14,192.176.0.0/12,192.192.0.0/10,193.0.0.0/8,194.0.0.0/7,196.0.0.0/6,200.0.0.0/5,"
                        + "208.0.0.0/4,224.0.0.0/3,::/1,8000::/2,c000::/3,e000::/4,f000::/5,f800::/6,fc00::/8,fe00::/7";
        }
        List<Peer> peers = new ArrayList<>();
        Peer.Builder builder = new Peer.Builder();
        builder.parseAllowedIPs(allowedIPs);
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

    public static String formatBytes(Context context, Long bytes) {
        String bytesString = "";
        if (bytes < 1024) {
            bytesString = String.format(context.getResources().getString(R.string.transfer_bytes),
                    String.format(Locale.getDefault(), "%.2f", (double) bytes));
        } else if (bytes < 1024 * 1024) {
            bytesString =
                    String.format(context.getResources().getString(R.string.transfer_kibibytes,
                            String.format(Locale.getDefault(), "%.2f", (double) (bytes / 1024.0))));
        } else if (bytes < 1024 * 1024 * 1024) {
            bytesString =
                    String.format(context.getResources().getString(R.string.transfer_mibibytes,
                            String.format(Locale.getDefault(), "%.2f",
                                    (double) (bytes / (1024.0 * 1024.0)))));
        } else if (bytes < 1024 * 1024 * 1024 * 1024L) {
            bytesString =
                    String.format(context.getResources().getString(R.string.transfer_gibibytes,
                            String.format(Locale.getDefault(), "%.2f",
                                    (double) (bytes / (1024.0 * 1024.0 * 1024.0)))));
        } else {
            bytesString =
                    String.format(context.getResources().getString(R.string.transfer_tibibytes,
                            String.format(Locale.getDefault(), "%.2f",
                                    (double) (bytes / (1024.0 * 1024.0 * 1024.0) / 1024.0))));
        }
        return bytesString;
    }
}
