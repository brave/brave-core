/**
 * Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser.custom_layout.popup_window_tooltip;

import static org.chromium.ui.base.ViewUtils.dpToPx;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;

import androidx.annotation.ColorInt;
import androidx.annotation.DimenRes;
import androidx.annotation.LayoutRes;

import org.chromium.base.Log;
import org.chromium.chrome.R;

// This class has many fields reported as unused:
// - mModal
// This class uses a builder, so all they seems introduced by intent.
@SuppressWarnings("UnusedVariable")
public class PopupWindowTooltip implements PopupWindow.OnDismissListener {
    private static final String TAG = PopupWindowTooltip.class.getSimpleName();

    // Default Resources
    private static final int DEFAULT_POPUP_WINDOW_STYLE_RES = android.R.attr.popupWindowStyle;
    private static final int DEFAULT_ARROW_COLOR_RES = R.color.shields_tooltip_arrow_color_1;
    private static final int DEFAULT_MARGIN_RES = R.dimen.shields_tooltip_margin;
    private static final int DEFAULT_PADDING_RES = R.dimen.shields_tooltip_padding;
    private static final int DEFAULT_ARROW_WIDTH_RES = R.dimen.shields_tooltip_arrow_width;
    private static final int DEFAULT_ARROW_HEIGHT_RES = R.dimen.shields_tooltip_arrow_height;

    private final Context mContext;
    private OnDismissListener mOnDismissListener;
    private OnShowListener mOnShowListener;
    private PopupWindow mPopupWindow;
    private final int mGravity;
    private final int mArrowDirection;
    private final boolean mDismissOnInsideTouch;
    private final boolean mDismissOnOutsideTouch;
    private final boolean mModal;
    private final boolean mBackgroundDimDisabled;
    private final boolean mContentArrowAtStart;
    private final View mContentView;
    private View mContentLayout;
    private final View mAnchorView;
    private final float mMaxWidth;
    private ViewGroup mRootView;
    private ImageView mArrowView;
    private final Drawable mArrowColorDrawable;
    private final float mMargin;
    private final float mPadding;
    private final float mArrowWidth;
    private final float mArrowHeight;
    private final int mParentPaddingHorizontal;
    private final int mParentPaddingVertical;
    private boolean mDismissed;
    private final int mWidth;
    private final int mHeight;

    private PopupWindowTooltip(Builder builder) {
        mContext = builder.mContext;
        mGravity = builder.mGravity;
        mArrowDirection = builder.mArrowDirection;
        mDismissOnInsideTouch = builder.mDismissOnInsideTouch;
        mDismissOnOutsideTouch = builder.mDismissOnOutsideTouch;
        mModal = builder.mModal;
        mContentView = builder.mContentView;
        mAnchorView = builder.mAnchorView;
        mMaxWidth = builder.mMaxWidth;
        mArrowWidth = builder.mArrowWidth;
        mArrowHeight = builder.mArrowHeight;
        mArrowColorDrawable = builder.mArrowColorDrawable;
        mMargin = builder.mMargin;
        mPadding = builder.mPadding;
        mOnDismissListener = builder.mOnDismissListener;
        mOnShowListener = builder.mOnShowListener;
        mBackgroundDimDisabled = builder.mBackgroundDimDisabled;
        mContentArrowAtStart = builder.mContentArrowAtStart;
        mParentPaddingHorizontal = builder.mParentPaddingHorizontal;
        mParentPaddingVertical = builder.mParentPaddingVertical;
        mRootView = PopupWindowTooltipUtils.findFrameLayout(mAnchorView);
        mWidth = builder.mWidth;
        mHeight = builder.mHeight;
        init();
    }

    private void init() {
        configPopupWindow();
        configContentView();
    }

    private void configPopupWindow() {
        mPopupWindow = new PopupWindow(mContext, null, DEFAULT_POPUP_WINDOW_STYLE_RES);
        mPopupWindow.setOnDismissListener(this);
        mPopupWindow.setWidth(mWidth);
        mPopupWindow.setHeight(mHeight);
        mPopupWindow.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        mPopupWindow.setOutsideTouchable(true);
        mPopupWindow.setTouchable(true);
        mPopupWindow.setTouchInterceptor(new View.OnTouchListener() {
            @SuppressLint("ClickableViewAccessibility")
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                final int x = (int) event.getX();
                final int y = (int) event.getY();

                if (!mDismissOnOutsideTouch && (event.getAction() == MotionEvent.ACTION_DOWN)
                        && ((x < 0) || (x >= mContentLayout.getMeasuredWidth()) || (y < 0)
                                || (y >= mContentLayout.getMeasuredHeight()))) {
                    return true;
                } else if (!mDismissOnOutsideTouch
                        && event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                    return true;
                } else if ((event.getAction() == MotionEvent.ACTION_DOWN)
                        && mDismissOnInsideTouch) {
                    dismiss();
                    return true;
                }
                return false;
            }
        });
        mPopupWindow.setClippingEnabled(false);
    }

    public void show() {
        verifyDismissed();

        mContentLayout.getViewTreeObserver().addOnGlobalLayoutListener(mLocationLayoutListener);
        mContentLayout.getViewTreeObserver().addOnGlobalLayoutListener(mAutoDismissLayoutListener);

        mRootView.post(
                new Runnable() {
                    @Override
                    public void run() {
                        if (mRootView.isShown()) {
                            mPopupWindow.showAtLocation(
                                    mRootView,
                                    Gravity.NO_GRAVITY,
                                    mRootView.getWidth(),
                                    mRootView.getHeight());
                            if (!mBackgroundDimDisabled) {
                                dimBackgroundPopupWindow();
                            }
                        } else {
                            Log.e(
                                    TAG,
                                    "Tooltip cannot be shown, root view is invalid or has been"
                                            + " closed.");
                        }
                    }
                });
    }

    private void dimBackgroundPopupWindow() {
        View container = mPopupWindow.getContentView().getRootView();
        Context context = mPopupWindow.getContentView().getContext();
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        WindowManager.LayoutParams p = (WindowManager.LayoutParams) container.getLayoutParams();
        p.flags |= WindowManager.LayoutParams.FLAG_DIM_BEHIND;
        p.dimAmount = 0.4f;
        wm.updateViewLayout(container, p);
    }

    private void verifyDismissed() {
        if (mDismissed) {
            throw new IllegalArgumentException("Tooltip has been mDismissed.");
        }
    }

    private PointF calculePopupLocation() {
        PointF location = new PointF();

        final RectF anchorRect = PopupWindowTooltipUtils.calculeRectInWindow(mAnchorView);
        final PointF anchorCenter = new PointF(anchorRect.centerX(), anchorRect.centerY());

        switch (mGravity) {
            case Gravity.START:
                location.x = anchorRect.left - mPopupWindow.getContentView().getWidth() - mMargin;
                location.y = anchorCenter.y - mPopupWindow.getContentView().getHeight() / 2f;
                break;
            case Gravity.END:
                location.x = anchorRect.right + mMargin;
                location.y = anchorCenter.y - mPopupWindow.getContentView().getHeight() / 2f;
                break;
            case Gravity.TOP:
                location.x = anchorCenter.x - mPopupWindow.getContentView().getWidth() / 2f;
                location.y = anchorRect.top - mPopupWindow.getContentView().getHeight() - mMargin;
                break;
            case Gravity.BOTTOM:
                location.x = mContentArrowAtStart
                        ? anchorRect.left
                        : anchorCenter.x - mPopupWindow.getContentView().getWidth() / 2f;
                location.y = anchorRect.bottom + mMargin;
                break;
            case Gravity.CENTER:
                location.x = anchorCenter.x - mPopupWindow.getContentView().getWidth() / 2f;
                location.y = anchorCenter.y - mPopupWindow.getContentView().getHeight() / 2f;
                break;
            default:
                throw new IllegalArgumentException(
                        "Gravity must have be CENTER, START, END, TOP or BOTTOM.");
        }

        return location;
    }

    private void configContentView() {
        mContentView.setPadding((int) mPadding, (int) mPadding, (int) mPadding, (int) mPadding);

        LinearLayout linearLayout = new LinearLayout(mContext);
        linearLayout.setLayoutParams(new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        linearLayout.setOrientation(mArrowDirection == ArrowColorDrawable.LEFT
                                || mArrowDirection == ArrowColorDrawable.RIGHT
                        ? LinearLayout.HORIZONTAL
                        : LinearLayout.VERTICAL);
        linearLayout.setPadding(mParentPaddingHorizontal, mParentPaddingVertical,
                mParentPaddingHorizontal, mParentPaddingVertical);

        mArrowView = new ImageView(mContext);
        mArrowView.setImageDrawable(mArrowColorDrawable);
        LinearLayout.LayoutParams arrowLayoutParams;

        if (mArrowDirection == ArrowColorDrawable.TOP
                || mArrowDirection == ArrowColorDrawable.BOTTOM) {
            arrowLayoutParams =
                    new LinearLayout.LayoutParams((int) mArrowWidth, (int) mArrowHeight, 0);
        } else {
            arrowLayoutParams =
                    new LinearLayout.LayoutParams((int) mArrowHeight, (int) mArrowWidth, 0);
        }

        arrowLayoutParams.gravity = Gravity.CENTER;
        mArrowView.setLayoutParams(arrowLayoutParams);

        if (mArrowDirection == ArrowColorDrawable.BOTTOM
                || mArrowDirection == ArrowColorDrawable.RIGHT) {
            linearLayout.addView(mContentView);
            linearLayout.addView(mArrowView);
        } else {
            linearLayout.addView(mArrowView);
            linearLayout.addView(mContentView);
        }

        LinearLayout.LayoutParams contentViewParams =
                new LinearLayout.LayoutParams(mWidth, mHeight, 0);
        contentViewParams.gravity = Gravity.CENTER;
        mContentView.setLayoutParams(contentViewParams);

        mContentLayout = linearLayout;
        mContentLayout.setVisibility(View.INVISIBLE);
        mPopupWindow.setContentView(mContentLayout);
    }

    public void dismiss() {
        if (mDismissed) return;

        mDismissed = true;
        if (mPopupWindow != null) {
            mPopupWindow.dismiss();
        }
    }
    public boolean isShowing() {
        return mPopupWindow != null && mPopupWindow.isShowing();
    }
    public <T extends View> T findViewById(int id) {
        // noinspection unchecked
        return (T) mContentLayout.findViewById(id);
    }

    @Override
    public void onDismiss() {
        mDismissed = true;
        mRootView = null;
        if (mOnDismissListener != null) mOnDismissListener.onDismiss(this);
        mOnDismissListener = null;
        mPopupWindow.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(
                mLocationLayoutListener);
        mPopupWindow.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(
                mArrowLayoutListener);
        mPopupWindow.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(
                mShowLayoutListener);
        mPopupWindow.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(
                mAutoDismissLayoutListener);
        mPopupWindow = null;
    }

    private final ViewTreeObserver.OnGlobalLayoutListener mLocationLayoutListener =
            new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    final PopupWindow popup = mPopupWindow;
                    if (popup == null || mDismissed) return;
                    if (mMaxWidth > 0 && mContentView.getWidth() > mMaxWidth) {
                        PopupWindowTooltipUtils.setWidth(mContentView, mMaxWidth);
                        popup.update(
                                ViewGroup.LayoutParams.WRAP_CONTENT,
                                ViewGroup.LayoutParams.WRAP_CONTENT);
                        return;
                    }
                    popup.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    popup.getContentView()
                            .getViewTreeObserver()
                            .addOnGlobalLayoutListener(mArrowLayoutListener);
                    PointF location = calculePopupLocation();
                    popup.setClippingEnabled(true);
                    popup.update(
                            (int) location.x,
                            (int) location.y,
                            popup.getWidth(),
                            popup.getHeight());
                    popup.getContentView().requestLayout();
                }
            };
    private final ViewTreeObserver.OnGlobalLayoutListener mArrowLayoutListener =
            new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    final PopupWindow popup = mPopupWindow;
                    if (popup == null || mDismissed) return;
                    popup.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    popup.getContentView()
                            .getViewTreeObserver()
                            .addOnGlobalLayoutListener(mShowLayoutListener);
                    RectF achorRect = PopupWindowTooltipUtils.calculeRectOnScreen(mAnchorView);
                    RectF contentViewRect =
                            PopupWindowTooltipUtils.calculeRectOnScreen(mContentLayout);
                    float x;
                    float y;
                    if (mArrowDirection == ArrowColorDrawable.TOP
                            || mArrowDirection == ArrowColorDrawable.BOTTOM) {
                        x = mContentLayout.getPaddingLeft() + dpToPx(mContext, 2);
                        if (mContentArrowAtStart) {
                            x = x + dpToPx(mContext, 20);
                        } else {
                            float centerX =
                                    (contentViewRect.width() / 2f) - (mArrowView.getWidth() / 2f);
                            float newX =
                                    centerX - (contentViewRect.centerX() - achorRect.centerX());
                            if (newX > x) {
                                if (newX + mArrowView.getWidth() + x > contentViewRect.width()) {
                                    x = contentViewRect.width() - mArrowView.getWidth() - x;
                                } else {
                                    x = newX;
                                }
                            }
                        }
                        y = mArrowView.getTop();
                        y = y + (mArrowDirection == ArrowColorDrawable.BOTTOM ? -1 : +1);
                    } else {
                        y = mContentLayout.getPaddingTop() + dpToPx(mContext, 2);
                        float centerY =
                                (contentViewRect.height() / 2f) - (mArrowView.getHeight() / 2f);
                        float newY = centerY - (contentViewRect.centerY() - achorRect.centerY());
                        if (newY > y) {
                            if (newY + mArrowView.getHeight() + y > contentViewRect.height()) {
                                y = contentViewRect.height() - mArrowView.getHeight() - y;
                            } else {
                                y = newY;
                            }
                        }
                        x = mArrowView.getLeft();
                        x = x + (mArrowDirection == ArrowColorDrawable.RIGHT ? -1 : +1);
                    }
                    mArrowView.setX((int) x);
                    mArrowView.setY((int) y);
                    popup.getContentView().requestLayout();
                }
            };
    private final ViewTreeObserver.OnGlobalLayoutListener mShowLayoutListener =
            new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    final PopupWindow popup = mPopupWindow;
                    if (popup == null || mDismissed) return;
                    popup.getContentView().getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    if (mOnShowListener != null) mOnShowListener.onShow(PopupWindowTooltip.this);
                    mOnShowListener = null;
                    mContentLayout.setVisibility(View.VISIBLE);
                }
            };
    private final ViewTreeObserver.OnGlobalLayoutListener mAutoDismissLayoutListener =
            new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    final PopupWindow popup = mPopupWindow;
                    if (popup == null || mDismissed) return;
                    if (!mRootView.isShown()) dismiss();
                }
            };

    public interface OnDismissListener {
        void onDismiss(PopupWindowTooltip tooltip);
    }
    public interface OnShowListener {
        void onShow(PopupWindowTooltip tooltip);
    }

    public static class Builder {
        private final Context mContext;
        private boolean mDismissOnInsideTouch = true;
        private boolean mDismissOnOutsideTouch = true;
        private boolean mModal;
        private boolean mBackgroundDimDisabled;
        private boolean mContentArrowAtStart;
        private View mContentView;
        private View mAnchorView;
        private int mArrowDirection = ArrowColorDrawable.AUTO;
        private int mGravity = Gravity.BOTTOM;
        private float mMaxWidth;
        private Drawable mArrowColorDrawable;
        private float mMargin = -1;
        private float mPadding = -1;
        private OnDismissListener mOnDismissListener;
        private OnShowListener mOnShowListener;
        private int mArrowColor;
        private float mArrowHeight;
        private float mArrowWidth;
        private int mParentPaddingHorizontal;
        private int mParentPaddingVertical;
        private int mWidth = ViewGroup.LayoutParams.WRAP_CONTENT;
        private int mHeight = ViewGroup.LayoutParams.WRAP_CONTENT;

        public Builder(Context context) {
            mContext = context;
        }

        public PopupWindowTooltip build() throws IllegalArgumentException {
            validateArguments();
            if (mArrowColor == 0) {
                mArrowColor = PopupWindowTooltipUtils.getColor(mContext, DEFAULT_ARROW_COLOR_RES);
            }
            if (mMargin < 0) {
                mMargin = mContext.getResources().getDimension(DEFAULT_MARGIN_RES);
            }
            if (mPadding < 0) {
                mPadding = mContext.getResources().getDimension(DEFAULT_PADDING_RES);
            }
            if (mArrowDirection == ArrowColorDrawable.AUTO) {
                mArrowDirection = PopupWindowTooltipUtils.tooltipGravityToArrowDirection(mGravity);
            }
            if (mArrowColorDrawable == null) {
                mArrowColorDrawable = new ArrowColorDrawable(mArrowColor, mArrowDirection);
            }
            if (mArrowWidth == 0) {
                mArrowWidth = mContext.getResources().getDimension(DEFAULT_ARROW_WIDTH_RES);
            }
            if (mArrowHeight == 0) {
                mArrowHeight = mContext.getResources().getDimension(DEFAULT_ARROW_HEIGHT_RES);
            }
            return new PopupWindowTooltip(this);
        }

        private void validateArguments() throws IllegalArgumentException {
            if (mContext == null) {
                throw new IllegalArgumentException("Context not specified.");
            }
            if (mAnchorView == null) {
                throw new IllegalArgumentException("Anchor view not specified.");
            }
        }

        public Builder setWidth(int width) {
            mWidth = width;
            return this;
        }

        public Builder setHeight(int height) {
            mHeight = height;
            return this;
        }

        public Builder contentView(@LayoutRes int contentViewId) {
            LayoutInflater inflater =
                    (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            mContentView = inflater.inflate(contentViewId, null, false);
            return this;
        }

        public Builder dismissOnInsideTouch(boolean dismissOnInsideTouch) {
            mDismissOnInsideTouch = dismissOnInsideTouch;
            return this;
        }

        public Builder dismissOnOutsideTouch(boolean dismissOnOutsideTouch) {
            mDismissOnOutsideTouch = dismissOnOutsideTouch;
            return this;
        }

        public Builder modal(boolean modal) {
            mModal = modal;
            return this;
        }

        public Builder backgroundDimDisabled(boolean backgroundDimDisabled) {
            mBackgroundDimDisabled = backgroundDimDisabled;
            return this;
        }

        public Builder contentArrowAtStart(boolean contentArrowAtStart) {
            mContentArrowAtStart = contentArrowAtStart;
            return this;
        }

        public Builder anchorView(View anchorView) {
            mAnchorView = anchorView;
            return this;
        }

        public Builder gravity(int gravity) {
            mGravity = gravity;
            return this;
        }

        public Builder arrowDirection(int arrowDirection) {
            mArrowDirection = arrowDirection;
            return this;
        }

        public Builder maxWidth(@DimenRes int maxWidthRes) {
            mMaxWidth = mContext.getResources().getDimension(maxWidthRes);
            return this;
        }

        public Builder maxWidth(float maxWidth) {
            mMaxWidth = maxWidth;
            return this;
        }

        public Builder padding(float padding) {
            mPadding = padding;
            return this;
        }

        public Builder padding(@DimenRes int paddingRes) {
            mPadding = mContext.getResources().getDimension(paddingRes);
            return this;
        }

        public Builder margin(float margin) {
            mMargin = margin;
            return this;
        }

        public Builder margin(@DimenRes int marginRes) {
            mMargin = mContext.getResources().getDimension(marginRes);
            return this;
        }

        public Builder parentPaddingHorizontal(int parentPaddingHorizontal) {
            mParentPaddingHorizontal = parentPaddingHorizontal;
            return this;
        }

        public Builder parentPaddingVertical(int parentPaddingVertical) {
            mParentPaddingVertical = parentPaddingVertical;
            return this;
        }

        public Builder arrowColor(@ColorInt int arrowColor) {
            mArrowColor = arrowColor;
            return this;
        }

        public Builder arrowHeight(float arrowHeight) {
            mArrowHeight = arrowHeight;
            return this;
        }

        public Builder arrowWidth(float arrowWidth) {
            mArrowWidth = arrowWidth;
            return this;
        }

        public Builder onDismissListener(OnDismissListener onDismissListener) {
            mOnDismissListener = onDismissListener;
            return this;
        }

        public Builder onShowListener(OnShowListener onShowListener) {
            mOnShowListener = onShowListener;
            return this;
        }
    }
}
