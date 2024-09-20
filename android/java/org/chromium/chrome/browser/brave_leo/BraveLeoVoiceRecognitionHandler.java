/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.speech.RecognizerIntent;
import android.text.TextUtils;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.content_public.browser.WebContents;
import org.chromium.ui.base.WindowAndroid;
import org.chromium.ui.permissions.PermissionCallback;

import java.util.ArrayList;
import java.util.List;

public class BraveLeoVoiceRecognitionHandler {
    private static final String TAG = "LeoVoiceRecognition";
    private WindowAndroid mWindowAndroid;
    private WebContents mContextWebContents;
    private String mConversationUuid;

    /** Callback for when we receive voice search results after initiating voice recognition. */
    class VoiceRecognitionCompleteCallback implements WindowAndroid.IntentCallback {
        private boolean mCallbackComplete;

        public VoiceRecognitionCompleteCallback() {}

        // WindowAndroid.IntentCallback implementation:
        @Override
        public void onIntentCompleted(int resultCode, Intent data) {
            if (mCallbackComplete) {
                return;
            }

            mCallbackComplete = true;
            if (resultCode != Activity.RESULT_OK || data.getExtras() == null) {
                return;
            }

            handleTranscriptionResult(data);
        }

        /**
         * Processes the transcription results within the given Intent.
         *
         * @param data The {@link Intent} with returned transcription data.
         */
        private void handleTranscriptionResult(Intent data) {
            List<BraveLeoVoiceRecognitionHandler.VoiceResult> voiceResults =
                    convertBundleToVoiceResults(data.getExtras());
            BraveLeoVoiceRecognitionHandler.VoiceResult topResult =
                    (voiceResults != null && voiceResults.size() > 0) ? voiceResults.get(0) : null;
            if (topResult == null) {
                return;
            }

            String topResultQuery = topResult.getMatch();
            if (TextUtils.isEmpty(topResultQuery)) {
                return;
            }
            BraveLeoUtils.openLeoQuery(
                    mContextWebContents, mConversationUuid, topResultQuery, false);
        }
    }

    /** A storage class that holds voice recognition string matches and confidence scores. */
    public static class VoiceResult {
        private final String mMatch;
        private final float mConfidence;

        public VoiceResult(String match, float confidence) {
            mMatch = match;
            mConfidence = confidence;
        }

        /**
         * @return The text match from the voice recognition.
         */
        public String getMatch() {
            return mMatch;
        }

        /**
         * @return The confidence value of the recognition that should go from 0.0 to 1.0.
         */
        public float getConfidence() {
            return mConfidence;
        }
    }

    public BraveLeoVoiceRecognitionHandler(
            WindowAndroid windowAndroid, WebContents contextWebContents, String conversationUuid) {
        mWindowAndroid = windowAndroid;
        mContextWebContents = contextWebContents;
        mConversationUuid = conversationUuid;
    }

    private List<BraveLeoVoiceRecognitionHandler.VoiceResult> convertBundleToVoiceResults(
            Bundle extras) {
        if (extras == null) return null;

        ArrayList<String> strings = extras.getStringArrayList(RecognizerIntent.EXTRA_RESULTS);
        float[] confidences = extras.getFloatArray(RecognizerIntent.EXTRA_CONFIDENCE_SCORES);

        if (strings == null || confidences == null) return null;
        if (strings.size() != confidences.length) return null;

        List<BraveLeoVoiceRecognitionHandler.VoiceResult> results = new ArrayList<>();
        for (int i = 0; i < strings.size(); ++i) {
            results.add(
                    new BraveLeoVoiceRecognitionHandler.VoiceResult(
                            strings.get(i), confidences[i]));
        }
        return results;
    }

    public void startVoiceRecognition() {
        ThreadUtils.assertOnUiThread();
        if (mWindowAndroid == null) {
            return;
        }
        Activity activity = mWindowAndroid.getActivity().get();
        if (activity == null) {
            return;
        }
        if (!startSystemForVoicePrompt(activity)) {
            Log.w(TAG, "Couldn't find suitable provider for voice searching");
        }
    }

    private boolean startSystemForVoicePrompt(Activity activity) {
        // Check if we need to request audio permissions. If we do, then a permissions
        // prompt will appear and startVoiceRecognition will be called again.
        if (!ensureAudioPermissionGranted(activity)) {
            return false;
        }

        Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(
                RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        intent.putExtra(
                RecognizerIntent.EXTRA_CALLING_PACKAGE,
                activity.getComponentName().flattenToString());
        intent.putExtra(RecognizerIntent.EXTRA_ENABLE_LANGUAGE_DETECTION, true);

        if (!showSpeechRecognitionIntent(intent)) {
            return false;
        }

        return true;
    }

    private boolean showSpeechRecognitionIntent(Intent intent) {
        return mWindowAndroid.showCancelableIntent(
                        intent,
                        new BraveLeoVoiceRecognitionHandler.VoiceRecognitionCompleteCallback(),
                        null)
                >= 0;
    }

    private boolean ensureAudioPermissionGranted(Activity activity) {
        if (mWindowAndroid.hasPermission(Manifest.permission.RECORD_AUDIO)) {
            return true;
        }
        // If we don't have permission and also can't ask, then there's no more work left
        if (!mWindowAndroid.canRequestPermission(Manifest.permission.RECORD_AUDIO)) {
            return false;
        }

        PermissionCallback callback =
                (permissions, grantResults) -> {
                    if (grantResults.length != 1
                            || grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                        return;
                    }

                    startSystemForVoicePrompt(activity);
                };
        mWindowAndroid.requestPermissions(
                new String[] {Manifest.permission.RECORD_AUDIO}, callback);

        return false;
    }
}
