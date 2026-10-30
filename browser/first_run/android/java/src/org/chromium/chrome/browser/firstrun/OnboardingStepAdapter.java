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

import androidx.recyclerview.widget.RecyclerView;

import com.airbnb.lottie.LottieAnimationView;
import com.google.android.material.checkbox.MaterialCheckBox;

import org.chromium.brave.browser.firstrun.R;
import org.chromium.build.annotations.NullMarked;

/**
 * RecyclerView adapter for the onboarding ViewPager2, inflating one layout per step and delegating
 * navigation requests to the host via {@link OnboardingNavigationListener}.
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

        void onDismiss();

        void onWebDiscoverPreferenceEnabled();

        void onCrashReportingPreferenceChanged(final boolean enabled);

        void onP3aPreferenceChanged(final boolean enabled);
    }

    private final int[] mSteps;

    private final int mHelpBraveSearchPosition;
    private final int mBlockInterruptionsPosition;
    private final int mCustomizeBravePosition;
    private final int mMakeBraveBetterPosition;

    private final SpannableString mWdpLearnMore;
    private final PageBounceAnimator mPageBounceAnimator;
    private final OnboardingNavigationListener mListener;

    private boolean mCrashReportingChecked;
    private boolean mCrashReportingManaged;
    private boolean mP3aChecked;
    private boolean mP3aManaged;
    private final boolean mSkipMakeBraveBetter;
    private final boolean mHasCustomizeStep;

    public OnboardingStepAdapter(
            final SpannableString wdpLearnMore,
            final String dayZeroVariant,
            final boolean skipMakeBraveBetter,
            final PageBounceAnimator pageBounceAnimator,
            final OnboardingNavigationListener listener) {
        mWdpLearnMore = wdpLearnMore;
        mSkipMakeBraveBetter = skipMakeBraveBetter;
        mHasCustomizeStep =
                DAY_ZERO_VARIANT_Y.equals(dayZeroVariant)
                        || DAY_ZERO_VARIANT_Z.equals(dayZeroVariant);
        mPageBounceAnimator = pageBounceAnimator;
        mListener = listener;

        mSteps =
                mHasCustomizeStep
                        ? new int[] {
                            R.layout.help_brave_search,
                            R.layout.block_interruptions,
                            R.layout.customize_brave,
                            R.layout.make_brave_better
                        }
                        : new int[] {
                            R.layout.help_brave_search,
                            R.layout.block_interruptions,
                            R.layout.make_brave_better
                        };

        mHelpBraveSearchPosition = 0;
        mBlockInterruptionsPosition = 1;
        mCustomizeBravePosition = mHasCustomizeStep ? 2 : -1;
        if (mSkipMakeBraveBetter) {
            mMakeBraveBetterPosition = -1;
        } else {
            mMakeBraveBetterPosition = mHasCustomizeStep ? 3 : 2;
        }
    }

    @Override
    public OnboardingBaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        final View view = inflater.inflate(mSteps[viewType], parent, false);

        if (viewType == mHelpBraveSearchPosition) {
            return new HelpBraveSearchViewHolder(
                    view,
                    mWdpLearnMore,
                    mBlockInterruptionsPosition,
                    mPageBounceAnimator,
                    mListener);
        }

        if (viewType == mBlockInterruptionsPosition) {
            // If Make Brave Better page needs to be skipped because both settings are managed
            // by policy the assigned valued will be -1 (check the logic in the constructor)
            // so the listener will call onDismiss.
            final int nextOnboardingPosition =
                    mHasCustomizeStep ? mCustomizeBravePosition : mMakeBraveBetterPosition;
            return new BlockInterruptionsViewHolder(
                    view, nextOnboardingPosition, mPageBounceAnimator, mListener);
        }

        if (viewType == mCustomizeBravePosition) {
            // TODO:
            throw new AssertionError("Customize Brave view holder is not implemented yet.");
        }

        if (viewType == mMakeBraveBetterPosition) {
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
    public int getItemViewType(int position) {
        return position;
    }

    @Override
    public int getItemCount() {
        return mSteps.length;
    }

    public void setCrashReportingChecked(final boolean checked) {
        if (mCrashReportingChecked == checked) {
            return;
        }
        mCrashReportingChecked = checked;
        notifyItemChanged(mMakeBraveBetterPosition);
    }

    public void setCrashReportingManaged(final boolean managed) {
        if (mCrashReportingManaged == managed) {
            return;
        }
        mCrashReportingManaged = managed;
        notifyItemChanged(mMakeBraveBetterPosition);
    }

    public void setP3aChecked(final boolean checked) {
        if (mP3aChecked == checked) {
            return;
        }
        mP3aChecked = checked;
        notifyItemChanged(mMakeBraveBetterPosition);
    }

    public void setP3aManaged(final boolean managed) {
        if (mP3aManaged == managed) {
            return;
        }
        mP3aManaged = managed;
        notifyItemChanged(mMakeBraveBetterPosition);
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
                final int blockInterruptionsPosition,
                final PageBounceAnimator pageBounceAnimator,
                final OnboardingNavigationListener listener) {
            super(itemView);
            mSubtitle = itemView.findViewById(R.id.brave_onboarding_subtitle);

            mSubtitle.setMovementMethod(LinkMovementMethod.getInstance());
            mSubtitle.setText(wdpLearnMore);

            mLater = itemView.findViewById(R.id.onboarding_later);
            mLater.setClipToOutline(true);
            mLater.setOnClickListener(
                    v -> pageBounceAnimator.animateToPosition(blockInterruptionsPosition));

            mSure = itemView.findViewById(R.id.onboarding_sure);
            mSure.setClipToOutline(true);
            mSure.setOnClickListener(
                    view -> {
                        listener.onWebDiscoverPreferenceEnabled();
                        pageBounceAnimator.animateToPosition(blockInterruptionsPosition);
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
