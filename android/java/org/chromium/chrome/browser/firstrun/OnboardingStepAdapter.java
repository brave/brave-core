/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.firstrun;

import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import com.airbnb.lottie.LottieAnimationView;
import com.google.android.material.checkbox.MaterialCheckBox;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;

/**
 * RecyclerView adapter for the variant-B onboarding ViewPager2, inflating one layout per step and
 * delegating navigation requests to the host via {@link OnboardingNavigationListener}.
 */
@NullMarked
public class OnboardingStepAdapter
        extends RecyclerView.Adapter<OnboardingStepAdapter.OnboardingBaseViewHolder> {

    /**
     * Listener used by onboarding step views to request a forward page change in the host.
     *
     * @see WelcomeOnboardingActivity
     */
    public interface OnboardingNavigationListener {
        void onRequestPageChange(final int position);

        void onDismiss();

        void onWebDiscoverPreferenceEnabled();
    }

    private static final int[] STEPS = {
        R.layout.help_brave_search, R.layout.block_interruptions, R.layout.make_brave_better
    };

    private final SpannableString mWdpLearnMore;
    private final OnboardingNavigationListener mListener;

    private boolean mCrashReportingChecked;
    private boolean mCrashReportingManaged;
    private boolean mP3aChecked;
    private boolean mP3aManaged;

    public OnboardingStepAdapter(
            final SpannableString wdpLearnMore, final OnboardingNavigationListener listener) {
        mWdpLearnMore = wdpLearnMore;
        mListener = listener;
    }

    @Override
    public OnboardingBaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        View view = inflater.inflate(STEPS[viewType], parent, false);
        return switch (viewType) {
            case 0 -> new HelpBraveSearchViewHolder(view, mWdpLearnMore, mListener);
            case 1 -> new BlockInterruptionsViewHolder(view, mListener);
            case 2 ->
                    new MakeBraveBetterViewHolder(
                            view,
                            mCrashReportingManaged,
                            mCrashReportingChecked,
                            mP3aManaged,
                            mP3aChecked,
                            mListener);
            default ->
                    throw new IllegalStateException(
                            String.format("View not found for view type %d.", viewType));
        };
    }

    @Override
    public void onBindViewHolder(OnboardingBaseViewHolder holder, int position) {
        /* Unused. */
    }

    @Override
    public void onViewRecycled(OnboardingBaseViewHolder holder) {
        holder.viewRecycled();
    }

    @Override
    public int getItemViewType(int position) {
        return position;
    }

    @Override
    public int getItemCount() {
        return STEPS.length;
    }

    public void setCrashReportingChecked(final boolean checked) {
        mCrashReportingChecked = checked;
    }

    public void setCrashReportingManaged(final boolean managed) {
        mCrashReportingManaged = managed;
    }

    public void setP3aChecked(final boolean checked) {
        mP3aChecked = checked;
    }

    public void setP3aManaged(final boolean managed) {
        mP3aManaged = managed;
    }

    abstract static class OnboardingBaseViewHolder extends RecyclerView.ViewHolder {

        public OnboardingBaseViewHolder(View itemView) {
            super(itemView);
        }

        public abstract void viewRecycled();
    }

    private static final class HelpBraveSearchViewHolder extends OnboardingBaseViewHolder {
        final TextView mSubtitle;
        final Button mLater;
        final Button mSure;

        private HelpBraveSearchViewHolder(
                final View itemView,
                final SpannableString wdpLearnMore,
                final OnboardingNavigationListener listener) {
            super(itemView);
            mSubtitle = itemView.findViewById(R.id.brave_onboarding_subtitle);

            mSubtitle.setMovementMethod(LinkMovementMethod.getInstance());
            mSubtitle.setText(wdpLearnMore);

            mLater = itemView.findViewById(R.id.onboarding_later);
            mLater.setClipToOutline(true);
            mLater.setOnClickListener(v -> listener.onRequestPageChange(1));

            mSure = itemView.findViewById(R.id.onboarding_sure);
            mSure.setClipToOutline(true);
            mSure.setOnClickListener(
                    view -> {
                        listener.onWebDiscoverPreferenceEnabled();
                        listener.onRequestPageChange(1);
                    });
        }

        @Override
        public void viewRecycled() {
            /* No-op. */
        }
    }

    private static final class BlockInterruptionsViewHolder extends OnboardingBaseViewHolder {
        final Button mContinue;
        final LottieAnimationView mBlockInterruptions;

        private BlockInterruptionsViewHolder(
                final View itemView, final OnboardingNavigationListener listener) {
            super(itemView);
            mContinue = itemView.findViewById(R.id.onboarding_continue);
            mContinue.setClipToOutline(true);
            mContinue.setOnClickListener(view -> listener.onRequestPageChange(2));

            mBlockInterruptions = itemView.findViewById(R.id.onboarding_illustration);
        }

        @Override
        public void viewRecycled() {
            mBlockInterruptions.cancelAnimation();
        }
    }

    private static final class MakeBraveBetterViewHolder extends OnboardingBaseViewHolder {
        final Button mStartBrowsing;
        final MaterialCheckBox mSendCrashReports;
        final MaterialCheckBox mSendP3a;

        private MakeBraveBetterViewHolder(
                final View itemView,
                final boolean crashReportingManaged,
                final boolean crashReportingChecked,
                final boolean p3aManaged,
                final boolean p3aChecked,
                final OnboardingNavigationListener listener) {
            super(itemView);
            mStartBrowsing = itemView.findViewById(R.id.onboarding_start_browsing);
            mStartBrowsing.setClipToOutline(true);
            mStartBrowsing.setOnClickListener(view -> listener.onDismiss());

            mSendCrashReports = itemView.findViewById(R.id.send_crash_reports);
            if (crashReportingManaged) {
                mSendCrashReports.setVisibility(View.INVISIBLE);
            } else {
                mSendCrashReports.setChecked(crashReportingChecked);
                mSendCrashReports.setOnCheckedChangeListener(
                        (buttonView, isChecked) ->
                                WelcomeOnboardingActivity.setMetricsReportingConsent(isChecked));
            }

            mSendP3a = itemView.findViewById(R.id.send_p3a_reports);
            if (p3aManaged) {
                mSendP3a.setVisibility(View.INVISIBLE);
            } else {
                mSendP3a.setChecked(p3aChecked);
                mSendP3a.setOnCheckedChangeListener(
                        (buttonView, isChecked) ->
                                WelcomeOnboardingActivity.setP3aConsent(isChecked));
            }
        }

        @Override
        public void viewRecycled() {
            /* No-op. */
        }
    }
}
