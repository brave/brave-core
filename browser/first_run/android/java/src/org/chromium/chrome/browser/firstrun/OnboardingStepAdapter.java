/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
package org.chromium.chrome.browser.firstrun;

import static org.chromium.chrome.browser.day_zero.DayZeroHelper.DAY_ZERO_VARIANT_Y;
import static org.chromium.chrome.browser.day_zero.DayZeroHelper.DAY_ZERO_VARIANT_Z;

import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.LayoutRes;
import androidx.recyclerview.widget.RecyclerView;

import com.airbnb.lottie.LottieAnimationView;
import com.google.android.material.checkbox.MaterialCheckBox;

import org.chromium.brave.browser.firstrun.R;
import org.chromium.build.annotations.NullMarked;

import java.util.ArrayList;
import java.util.List;

/**
 * RecyclerView adapter for the onboarding ViewPager2, inflating one layout per step and delegating
 * navigation requests to the host via {@link OnboardingNavigationListener}.
 */
@NullMarked
public class OnboardingStepAdapter
        extends RecyclerView.Adapter<OnboardingStepAdapter.OnboardingBaseViewHolder> {

    /**
     * Listener used by onboarding step views to dismiss the onboarding and to report preference
     * changes to the host.
     *
     * @see WelcomeOnboardingActivity
     */
    public interface OnboardingNavigationListener {
        void onDismiss();

        void onWebDiscoverPreferenceEnabled();

        void onCrashReportingPreferenceChanged(final boolean enabled);

        void onP3aPreferenceChanged(final boolean enabled);
    }

    /** Layouts of the visible steps, in display order. */
    private final List<Integer> mSteps = new ArrayList<>();

    private final SpannableString mWdpLearnMore;
    private final PageBounceAnimator mPageBounceAnimator;
    private final OnboardingNavigationListener mListener;

    private boolean mCrashReportingChecked;
    private boolean mCrashReportingManaged;
    private boolean mP3aChecked;
    private boolean mP3aManaged;

    public OnboardingStepAdapter(
            final SpannableString wdpLearnMore,
            final String dayZeroVariant,
            final boolean skipMakeBraveBetter,
            final PageBounceAnimator pageBounceAnimator,
            final OnboardingNavigationListener listener) {
        mWdpLearnMore = wdpLearnMore;
        mPageBounceAnimator = pageBounceAnimator;
        mListener = listener;

        mSteps.add(R.layout.help_brave_search);
        mSteps.add(R.layout.block_interruptions);
        if (DAY_ZERO_VARIANT_Y.equals(dayZeroVariant)
                || DAY_ZERO_VARIANT_Z.equals(dayZeroVariant)) {
            mSteps.add(R.layout.customize_brave);
        }
        if (!skipMakeBraveBetter) {
            mSteps.add(R.layout.make_brave_better);
        }
    }

    @Override
    public OnboardingBaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        final View view = inflater.inflate(viewType, parent, false);

        if (viewType == R.layout.help_brave_search) {
            return new HelpBraveSearchViewHolder(
                    view, mWdpLearnMore, getNextPosition(viewType), mPageBounceAnimator, mListener);
        }

        if (viewType == R.layout.block_interruptions) {
            // When it is the last step the next position is -1 and the view holder dismisses.
            return new BlockInterruptionsViewHolder(
                    view, getNextPosition(viewType), mPageBounceAnimator, mListener);
        }

        if (viewType == R.layout.customize_brave) {
            // TODO: GitHub issue https://github.com/brave/brave-browser/issues/60341.
            throw new AssertionError("Customize Brave view holder is not implemented yet.");
        }

        if (viewType == R.layout.make_brave_better) {
            return new MakeBraveBetterViewHolder(
                    view,
                    mCrashReportingManaged,
                    mCrashReportingChecked,
                    mP3aManaged,
                    mP3aChecked,
                    mListener);
        }

        throw new IllegalStateException(
                String.format("View not found for view type %d.", viewType));
    }

    @Override
    public void onBindViewHolder(OnboardingBaseViewHolder holder, int position) {
        if (holder instanceof MakeBraveBetterViewHolder makeBraveBetterViewHolder) {
            makeBraveBetterViewHolder.bind(
                    mCrashReportingManaged, mCrashReportingChecked, mP3aManaged, mP3aChecked);
        }
    }

    @Override
    public void onViewRecycled(OnboardingBaseViewHolder holder) {
        holder.viewRecycled();
    }

    @Override
    public @LayoutRes int getItemViewType(int position) {
        return mSteps.get(position);
    }

    @Override
    public int getItemCount() {
        return mSteps.size();
    }

    public void setCrashReportingChecked(final boolean checked) {
        if (mCrashReportingChecked == checked) {
            return;
        }
        mCrashReportingChecked = checked;
        notifyStepChanged(R.layout.make_brave_better);
    }

    public void setCrashReportingManaged(final boolean managed) {
        if (mCrashReportingManaged == managed) {
            return;
        }
        mCrashReportingManaged = managed;
        notifyStepChanged(R.layout.make_brave_better);
    }

    public void setP3aChecked(final boolean checked) {
        if (mP3aChecked == checked) {
            return;
        }
        mP3aChecked = checked;
        notifyStepChanged(R.layout.make_brave_better);
    }

    public void setP3aManaged(final boolean managed) {
        if (mP3aManaged == managed) {
            return;
        }
        mP3aManaged = managed;
        notifyStepChanged(R.layout.make_brave_better);
    }

    /** Returns the position after the given step, or -1 when it is the last one. */
    private int getNextPosition(@LayoutRes final int step) {
        final int next = mSteps.indexOf(step) + 1;
        return next < mSteps.size() ? next : -1;
    }

    private void notifyStepChanged(@LayoutRes final int step) {
        final int position = mSteps.indexOf(step);
        if (position != -1) {
            notifyItemChanged(position);
        }
    }

    public abstract static class OnboardingBaseViewHolder extends RecyclerView.ViewHolder {

        public OnboardingBaseViewHolder(View itemView) {
            super(itemView);
            itemView.setClipToOutline(true);
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
                final int nextPosition,
                final PageBounceAnimator pageBounceAnimator,
                final OnboardingNavigationListener listener) {
            super(itemView);
            mSubtitle = itemView.findViewById(R.id.brave_onboarding_subtitle);

            mSubtitle.setMovementMethod(LinkMovementMethod.getInstance());
            mSubtitle.setText(wdpLearnMore);

            mLater = itemView.findViewById(R.id.onboarding_later);
            mLater.setClipToOutline(true);
            mLater.setOnClickListener(v -> pageBounceAnimator.animateToPosition(nextPosition));

            mSure = itemView.findViewById(R.id.onboarding_sure);
            mSure.setClipToOutline(true);
            mSure.setOnClickListener(
                    view -> {
                        listener.onWebDiscoverPreferenceEnabled();
                        pageBounceAnimator.animateToPosition(nextPosition);
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
                final View itemView,
                final int nextOnboardingPosition,
                final PageBounceAnimator pageBounceAnimator,
                final OnboardingNavigationListener listener) {
            super(itemView);
            mContinue = itemView.findViewById(R.id.onboarding_continue);
            mContinue.setClipToOutline(true);
            mContinue.setOnClickListener(
                    view -> {
                        if (nextOnboardingPosition == -1) {
                            listener.onDismiss();
                        } else {
                            pageBounceAnimator.animateToPosition(nextOnboardingPosition);
                        }
                    });

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
                                listener.onCrashReportingPreferenceChanged(isChecked));
            }

            mSendP3a = itemView.findViewById(R.id.send_p3a_reports);
            if (p3aManaged) {
                mSendP3a.setVisibility(View.INVISIBLE);
            } else {
                mSendP3a.setChecked(p3aChecked);
                mSendP3a.setOnCheckedChangeListener(
                        (buttonView, isChecked) -> listener.onP3aPreferenceChanged(isChecked));
            }
        }

        private void bind(
                boolean crashReportingManaged,
                boolean crashReportingChecked,
                boolean p3aManaged,
                boolean p3aChecked) {
            if (crashReportingManaged) {
                mSendCrashReports.setVisibility(View.INVISIBLE);
            } else {
                mSendCrashReports.setVisibility(View.VISIBLE);
                mSendCrashReports.setChecked(crashReportingChecked);
            }

            if (p3aManaged) {
                mSendP3a.setVisibility(View.INVISIBLE);
            } else {
                mSendP3a.setVisibility(View.VISIBLE);
                mSendP3a.setChecked(p3aChecked);
            }
        }

        @Override
        public void viewRecycled() {
            /* No-op. */
        }
    }
}
