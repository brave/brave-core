package org.chromium.chrome.browser.vpn.wireguard;

import android.app.ActivityManager;
import android.content.Context;
import android.content.SharedPreferences;
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

    public static Interface getInterface() throws BadConfigException {
        Interface.Builder builder = new Interface.Builder();
        builder.parseAddresses("10.99.72.81/32");
        builder.parseDnsServers("1.1.1.1, 1.0.0.1");
        builder.parseListenPort("51821");
        builder.parsePrivateKey("eOVGdgGOcwfo7dyptSL3BMtHJTxmwdUIS/meDLZUx3g=");
        return builder.build();
    }

    public static List<Peer> getPeers() throws BadConfigException {
        List<Peer> peers = new ArrayList<>();
        Peer.Builder builder = new Peer.Builder();
        builder.parseAllowedIPs("0.0.0.0/0");
        builder.parseEndpoint("wg-testing-1-yyz.guardianapp.com:51821");
        builder.parsePublicKey("T2o1A8pIX+jUEthALK2j+17+KgoNblyUWqUDJT539QU=");
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
