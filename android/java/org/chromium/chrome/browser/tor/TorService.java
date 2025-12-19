/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.tor;

import android.content.Context;
import android.util.Log;

import org.chromium.base.ContextUtils;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

/**
 * TorService manages the Tor daemon lifecycle for Private Window with Tor.
 * It launches the Tor executable and provides SOCKS5 proxy at 127.0.0.1:9050.
 */
public class TorService {
    private static final String TAG = "TorService";

    // SOCKS5 proxy configuration
    public static final String TOR_SOCKS_HOST = "127.0.0.1";
    public static final int TOR_SOCKS_PORT = 9050;
    public static final String TOR_SOCKS_PROXY = "socks5://" + TOR_SOCKS_HOST + ":" + TOR_SOCKS_PORT;

    // Tor binary and data paths
    private static final String TOR_BINARY_NAME = "libTor.so";
    private static final String TOR_DATA_DIR = "tor_data";
    private static final String TORRC_FILE = "torrc";

    private static TorService sInstance;

    private Process mTorProcess;
    private TorConnectionState mConnectionState = TorConnectionState.DISCONNECTED;
    private final List<TorConnectionListener> mListeners = new ArrayList<>();

    /**
     * Connection states for Tor
     */
    public enum TorConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    }

    /**
     * Listener for Tor connection state changes
     */
    public interface TorConnectionListener {
        void onTorConnectionStateChanged(TorConnectionState state);

        void onTorLogMessage(String message);
    }

    private TorService() {
    }

    /**
     * Get the singleton instance of TorService
     */
    public static synchronized TorService getInstance() {
        if (sInstance == null) {
            sInstance = new TorService();
        }
        return sInstance;
    }

    /**
     * Check if Tor is currently running
     */
    public boolean isRunning() {
        return mTorProcess != null && mTorProcess.isAlive();
    }

    /**
     * Get current connection state
     */
    public TorConnectionState getConnectionState() {
        return mConnectionState;
    }

    /**
     * Check if Tor is connected
     */
    public boolean isConnected() {
        return mConnectionState == TorConnectionState.CONNECTED;
    }

    /**
     * Add a listener for connection state changes
     */
    public void addListener(TorConnectionListener listener) {
        if (!mListeners.contains(listener)) {
            mListeners.add(listener);
        }
    }

    /**
     * Remove a listener
     */
    public void removeListener(TorConnectionListener listener) {
        mListeners.remove(listener);
    }

    /**
     * Start the Tor daemon
     */
    public void startTor() {
        if (isRunning()) {
            Log.d(TAG, "Tor is already running");
            return;
        }

        setConnectionState(TorConnectionState.CONNECTING);

        // Initialize JNI bridge
        TorServiceBridge.getInstance().initialize();

        PostTask.postTask(TaskTraits.BEST_EFFORT_MAY_BLOCK, () -> {
            try {
                Context context = ContextUtils.getApplicationContext();

                // Setup Tor files
                File torBinary = setupTorBinary(context);
                File torDataDir = setupTorDataDir(context);
                File torrcFile = createTorrcFile(context, torDataDir);

                // Build command to launch Tor
                ProcessBuilder processBuilder = new ProcessBuilder(
                        torBinary.getAbsolutePath(),
                        "-f", torrcFile.getAbsolutePath());

                processBuilder.directory(torDataDir);
                processBuilder.redirectErrorStream(true);

                // Start Tor process
                mTorProcess = processBuilder.start();
                Log.i(TAG, "Tor process started with PID");

                // Monitor Tor output for connection status
                monitorTorOutput(mTorProcess);

            } catch (Exception e) {
                Log.e(TAG, "Failed to start Tor", e);
                setConnectionState(TorConnectionState.DISCONNECTED);
            }
        });
    }

    /**
     * Stop the Tor daemon
     */
    public void stopTor() {
        if (mTorProcess != null) {
            mTorProcess.destroy();
            mTorProcess = null;
            Log.i(TAG, "Tor process stopped");
        }
        setConnectionState(TorConnectionState.DISCONNECTED);

        // Destroy JNI bridge
        TorServiceBridge.getInstance().destroy();
    }

    /**
     * Request a new Tor circuit (new identity)
     */
    public void newIdentity() {
        Log.d(TAG, "New identity requested");
        // Request new circuit via JNI bridge
        TorServiceBridge.getInstance().setNewTorCircuit("about:blank");
    }

    /**
     * Setup the Tor binary - copy from assets if needed
     */
    private File setupTorBinary(Context context) throws IOException {
        File torBinary = new File(context.getFilesDir(), TOR_BINARY_NAME);

        // Copy the native library to executable location if not exists
        if (!torBinary.exists()) {
            // The library is in the native libs directory
            String nativeLibDir = context.getApplicationInfo().nativeLibraryDir;
            File sourceFile = new File(nativeLibDir, TOR_BINARY_NAME);

            if (sourceFile.exists()) {
                copyFile(sourceFile, torBinary);
            } else {
                throw new IOException("Tor binary not found at: " + sourceFile.getAbsolutePath());
            }
        }

        // Make executable
        if (!torBinary.canExecute()) {
            torBinary.setExecutable(true);
        }

        return torBinary;
    }

    /**
     * Setup the Tor data directory
     */
    private File setupTorDataDir(Context context) {
        File torDataDir = new File(context.getFilesDir(), TOR_DATA_DIR);
        if (!torDataDir.exists()) {
            torDataDir.mkdirs();
        }
        return torDataDir;
    }

    /**
     * Create the torrc configuration file
     */
    private File createTorrcFile(Context context, File dataDir) throws IOException {
        File torrcFile = new File(dataDir, TORRC_FILE);

        String torrcContent = "SocksPort " + TOR_SOCKS_PORT + "\n" +
                "DataDirectory " + dataDir.getAbsolutePath() + "\n" +
                "Log notice stdout\n" +
                "RunAsDaemon 0\n" +
                "AvoidDiskWrites 1\n" +
                "ClientOnly 1\n";

        try (FileOutputStream fos = new FileOutputStream(torrcFile)) {
            fos.write(torrcContent.getBytes());
        }

        return torrcFile;
    }

    /**
     * Monitor Tor process output for connection status
     */
    private void monitorTorOutput(Process process) {
        new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(process.getInputStream()))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    final String logLine = line;
                    Log.d(TAG, "Tor: " + logLine);

                    // Notify listeners of log messages
                    notifyLogMessage(logLine);

                    // Check for connection established
                    if (line.contains("Bootstrapped 100%") ||
                            line.contains("circuit established")) {
                        setConnectionState(TorConnectionState.CONNECTED);
                    }
                }
            } catch (IOException e) {
                Log.e(TAG, "Error reading Tor output", e);
            }

            // Process ended
            if (mConnectionState != TorConnectionState.DISCONNECTED) {
                setConnectionState(TorConnectionState.DISCONNECTED);
            }
        }, "TorOutputMonitor").start();
    }

    /**
     * Copy a file
     */
    private void copyFile(File source, File dest) throws IOException {
        try (InputStream is = new java.io.FileInputStream(source);
                FileOutputStream fos = new FileOutputStream(dest)) {
            byte[] buffer = new byte[8192];
            int length;
            while ((length = is.read(buffer)) > 0) {
                fos.write(buffer, 0, length);
            }
        }
    }

    /**
     * Set connection state and notify listeners
     */
    private void setConnectionState(TorConnectionState state) {
        mConnectionState = state;

        // Update proxy URI in JNI bridge when connected
        if (state == TorConnectionState.CONNECTED) {
            TorServiceBridge.getInstance().updateProxyUri(TOR_SOCKS_PROXY);
        }

        PostTask.postTask(TaskTraits.UI_DEFAULT, () -> {
            for (TorConnectionListener listener : mListeners) {
                listener.onTorConnectionStateChanged(state);
            }
        });
    }

    /**
     * Notify listeners of log messages
     */
    private void notifyLogMessage(String message) {
        PostTask.postTask(TaskTraits.UI_DEFAULT, () -> {
            for (TorConnectionListener listener : mListeners) {
                listener.onTorLogMessage(message);
            }
        });
    }
}
