package org.chromium.chrome.browser.vpn.wireguard;

import static org.chromium.chrome.browser.vpn.wireguard.WireguardUtils.getInterface;
import static org.chromium.chrome.browser.vpn.wireguard.WireguardUtils.getPeers;

import android.content.Context;

import com.wireguard.config.BadConfigException;
import com.wireguard.config.Config;

import org.chromium.base.Log;
import org.chromium.chrome.browser.vpn.wireguard.TunnelModel;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class WireguardConfigUtils {
    public static Config createConfig(Context context, String address, String host,
            String clientPrivateKey, String serverPublicKey)
            throws IOException, BadConfigException {
        File file = fileForConfig(context);
        if (!file.createNewFile()) {
            throw new IOException("Configuration file already exists");
        }
        FileOutputStream fileOutputStream = new FileOutputStream(file, false);
        Config config = new Config.Builder()
                                .setInterface(getInterface(address, clientPrivateKey))
                                .addPeers(getPeers(host, serverPublicKey))
                                .build();
        //     String configStr = config.toWgQuickString();
        //     configStr = configStr+"Endpoint = "+host+":51821";
        //     // configStr = configStr+"PublicKey = "+serverPublicKey;
        //     configStr = "[Interface]\n"+
        // "Address = 10.99.120.35/32\n"+
        // "DNS = 1.1.1.1, 1.0.0.1\n"+
        // "ListenPort = 51821\n"+
        // "PrivateKey = kCqFqJy57U+GAV2I0rE9N7Z1MhI8FCLox5Z035sUvl4=\n\n"+

        // "[Peer]\n"+
        // "AllowedIPs = 0.0.0.0/0\n"+
        // "Endpoint = toronto-ipsec-8.guardianapp.com:51821\n"+
        // "PublicKey = Nv45MLfckg0ppfspZRFb+1ZdlQtDUYw8EYc3lsFoxV0=";
        //     Log.e("BraveVPN", "Config : "+configStr);
        //     fileOutputStream.write(configStr.getBytes(StandardCharsets.UTF_8));
        fileOutputStream.write(config.toWgQuickString().getBytes(StandardCharsets.UTF_8));
        return config;
    }

    public static Config createConfig(Context context) throws IOException, BadConfigException {
        File file = fileForConfig(context);
        if (!file.createNewFile()) {
            throw new IOException("Configuration file already exists");
        }
        FileOutputStream fileOutputStream = new FileOutputStream(file, false);
        Config config =
                new Config.Builder().setInterface(getInterface()).addPeers(getPeers()).build();
        fileOutputStream.write(config.toWgQuickString().getBytes(StandardCharsets.UTF_8));
        return config;
    }

    public static void deleteConfig(Context context) throws Exception {
        File file = fileForConfig(context);
        if (!file.delete()) {
            throw new IOException("Cannot delete configuration file");
        }
    }

    public static Config loadConfig(Context context) throws Exception {
        FileInputStream fileInputStream = new FileInputStream(fileForConfig(context));
        return Config.parse(fileInputStream);
    }

    public static boolean isConfigExist(Context context) {
        return fileForConfig(context).exists();
    }

    private static File fileForConfig(Context context) {
        return new File(context.getFilesDir(), TunnelModel.TUNNEL_NAME + ".conf");
    }
}
