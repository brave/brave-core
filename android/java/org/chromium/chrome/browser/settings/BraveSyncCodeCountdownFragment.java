/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.settings;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.style.StyleSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.fragment.app.Fragment;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveSyncWorker;

import java.time.Duration;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneOffset;
import java.util.Timer;
import java.util.TimerTask;

public class BraveSyncCodeCountdownFragment extends Fragment {
    private Instant mNotAfter;
    private boolean mDestroyed;
    private Runnable mExpiredRunnable;
    private Timer mTextUpdater;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_sync_code_countdown, container, false);
    }

    @Override
    public void onDestroyView() {
        mDestroyed = true;
        super.onDestroyView();
    }

    public void setNotAfter(LocalDateTime notAfter) {
        mNotAfter = notAfter.toInstant(ZoneOffset.UTC);
        scheduleTextUpdate();
    }

    void scheduleTextUpdate() {
        cleanupTextUpdater();

        // There is no Chromium's option for repeating action, so using java.util.Timer.
        mTextUpdater = new Timer();
        mTextUpdater.schedule(
                new TimerTask() {
                    @Override
                    public void run() {
                        Activity activity = getActivity();
                        if (activity != null) {
                            activity.runOnUiThread(
                                    new Runnable() {
                                        @Override
                                        public void run() {
                                            updateText();
                                        }
                                    });
                        }
                    }
                },
                0,
                1000);
    }

    // duration.getSeconds should be toSeconds after api 31.
    @SuppressWarnings("JavaDurationGetSecondsToToSeconds")
    void updateText() {
        if (mDestroyed) {
            cleanupTextUpdater();
            return;
        }

        Duration duration = Duration.between(Instant.now(), mNotAfter);
        View countDownExpiredBlock = getView().findViewById(R.id.brave_sync_code_expired_block);
        View countdownBlock = getView().findViewById(R.id.brave_sync_code_countdown_block);

        if (duration.getSeconds() > 0) {
            countDownExpiredBlock.setVisibility(View.GONE);
            countdownBlock.setVisibility(View.VISIBLE);
            String formattedDuration =
                    BraveSyncWorker.get().getFormattedTimeDelta(duration.getSeconds());
            String theWarningText =
                    getResources()
                            .getString(
                                    R.string.brave_sync_valid_for_period_text, formattedDuration);

            SpannableString formatedText = new SpannableString(theWarningText);
            formatedText.setSpan(
                    new StyleSpan(Typeface.BOLD),
                    theWarningText.length() - formattedDuration.length(),
                    theWarningText.length(),
                    0);

            TextView countDownTextView =
                    (TextView) getView().findViewById(R.id.brave_sync_count_down_text);
            countDownTextView.setText(formatedText);
        } else {
            countDownExpiredBlock.setVisibility(View.VISIBLE);
            countdownBlock.setVisibility(View.GONE);
            assert mExpiredRunnable != null;
            mExpiredRunnable.run();
            cleanupTextUpdater();
        }
    }

    void setExpiredRunnable(Runnable expiredRunnable) {
        mExpiredRunnable = expiredRunnable;
    }

    private void cleanupTextUpdater() {
        if (mTextUpdater != null) {
            mTextUpdater.cancel();
            mTextUpdater.purge();
            mTextUpdater = null;
        }
    }
}
