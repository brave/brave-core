/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.slidingpanel

import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.PixelFormat
import android.graphics.Rect
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.os.Parcelable
import android.util.AttributeSet
import android.view.Gravity
import android.view.MotionEvent
import android.view.SoundEffectConstants
import android.view.View
import android.view.View.OnClickListener
import android.view.ViewGroup
import android.view.accessibility.AccessibilityEvent
import android.view.animation.Interpolator
import androidx.core.content.res.ResourcesCompat
import androidx.core.view.ViewCompat
import org.chromium.chrome.R
import java.util.concurrent.CopyOnWriteArrayList
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

class BottomPanelLayout @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyle: Int = 0
) :
    ViewGroup(context, attrs, defStyle) {
    private var minFlingVelocity = DEFAULT_MIN_FLING_VELOCITY
    private var mCoveredFadeColor = DEFAULT_FADE_COLOR
    private val mCoveredFadePaint = Paint()
    private var mShadowDrawable: Drawable? = null
    private var mPanelHeight = -1
    private var mShadowHeight = -1
    private var mParallaxOffset = -1
    private var mIsSlidingUp = false
    private var isOverlayed = DEFAULT_OVERLAY_FLAG
    private var isClipPanel = DEFAULT_CLIP_PANEL_FLAG
    private var mDragView: View? = null
    private var mDragViewResId = -1
    private var mScrollableView: View? = null
    private var mScrollableViewResId = 0
    private var mScrollableViewHelper = ScrollableViewHelper()
    private var mSlideableView: View? = null
    private var mMainView: View? = null

    enum class PanelState {
        EXPANDED, COLLAPSED, ANCHORED, HIDDEN, DRAGGING
    }

    var mSlideState: PanelState? = DEFAULT_SLIDE_STATE
    private var mLastNotDraggingSlideState: PanelState? = DEFAULT_SLIDE_STATE
    private var mSlideOffset = 0f
    private var mSlideRange = 0
    private var mAnchorPoint = 1f
    private var mIsUnableToDrag = false

    private var mIsTouchEnabled: Boolean
    private var mPrevMotionX = 0f
    private var mPrevMotionY = 0f
    private var mInitialMotionX = 0f
    private var mInitialMotionY = 0f
    private var mIsScrollableViewHandlingTouch = false
    private val mPanelSlideListeners: MutableList<PanelSlideListener> = CopyOnWriteArrayList()
    private var mFadeOnClickListener: OnClickListener? = null
    private var mDragHelper: ViewDragHelper?

    private var mFirstLayout = true
    private val mTmpRect = Rect()

    interface PanelSlideListener {
        fun onPanelSlide(panel: View?, slideOffset: Float) {}
        fun onPanelStateChanged(panel: View?, previousState: PanelState?, newState: PanelState?) {}
    }

    init {
        val scrollerInterpolator: Interpolator? = null
        if (attrs != null) {
            val defAttrs = context.obtainStyledAttributes(attrs, DEFAULT_ATTRS)
            val gravity = defAttrs.getInt(0, Gravity.NO_GRAVITY)
            setGravity(gravity)
            defAttrs.recycle()
            val ta = context.obtainStyledAttributes(attrs, R.styleable.BottomPanelLayout)
            mPanelHeight =
                ta.getDimensionPixelSize(R.styleable.BottomPanelLayout_panelHeight, -1)
            mScrollableViewResId =
                ta.getResourceId(R.styleable.BottomPanelLayout_scrollableView, -1)
            mSlideState = PanelState.entries.toTypedArray()[ta.getInt(
                R.styleable.BottomPanelLayout_initialState,
                DEFAULT_SLIDE_STATE.ordinal
            )]
            ta.recycle()
        }
        val density = context.resources.displayMetrics.density
        if (mPanelHeight == -1) {
            mPanelHeight = (DEFAULT_PANEL_HEIGHT * density + 0.5f).toInt()
        }
        if (mShadowHeight == -1) {
            mShadowHeight = (DEFAULT_SHADOW_HEIGHT * density + 0.5f).toInt()
        }
        if (mParallaxOffset == -1) {
            mParallaxOffset = (DEFAULT_PARALLAX_OFFSET * density).toInt()
        }
        // If the shadow height is zero, don't show the shadow
        mShadowDrawable = if (mShadowHeight > 0) {
            ResourcesCompat.getDrawable(
                resources,
                if (mIsSlidingUp) R.drawable.above_shadow else R.drawable.below_shadow,
                null
            )
        } else {
            null
        }
        setWillNotDraw(false)
        mDragHelper = ViewDragHelper.create(this, 0.5f, scrollerInterpolator, DragHelperCallback())
        mDragHelper?.minVelocity = minFlingVelocity * density
        mIsTouchEnabled = true
    }

    override fun onFinishInflate() {
        super.onFinishInflate()
        if (mDragViewResId != -1) {
            setDragView(findViewById(mDragViewResId))
        }
        if (mScrollableViewResId != -1) {
            setScrollableView(findViewById(mScrollableViewResId))
        }
    }

    private fun setGravity(gravity: Int) {
        require(!(gravity != Gravity.TOP && gravity != Gravity.BOTTOM)) { "gravity must be set to either top or bottom" }
        mIsSlidingUp = gravity == Gravity.BOTTOM
        if (!mFirstLayout) {
            requestLayout()
        }
    }

    private var isTouchEnabled: Boolean
        get() = mIsTouchEnabled && mSlideableView != null && mSlideState != PanelState.HIDDEN
        set(enabled) {
            mIsTouchEnabled = enabled
        }

    fun smoothToBottom() {
        if (!isEnabled || mSlideableView == null) {
            // Nothing to do.
            return
        }
        val panelTop = computePanelTopPosition(0f)
        if (mSlideableView?.left?.let {
                mDragHelper?.smoothSlideViewTo(
                    mSlideableView,
                    it, panelTop
                )
            } == true) {
            setAllChildrenVisible()
        }
    }

    var panelHeight: Int
        get() = mPanelHeight
        set(value) {
            if (panelHeight == value) {
                return
            }
            mPanelHeight = value
            if (!mFirstLayout) {
                requestLayout()
            }
            if (panelState == PanelState.COLLAPSED) {
                smoothToBottom()
                invalidate()
                return
            }
        }// Clamp slide offset at zero for parallax computation;

    private val currentParallaxOffset: Int
        get() {
            // Clamp slide offset at zero for parallax computation;
            val offset = (mParallaxOffset * max(mSlideOffset, 0f)).toInt()
            return if (mIsSlidingUp) -offset else offset
        }

    fun addPanelSlideListener(listener: PanelSlideListener) {
        synchronized(mPanelSlideListeners) { mPanelSlideListeners.add(listener) }
    }

    fun removePanelSlideListener(listener: PanelSlideListener) {
        synchronized(mPanelSlideListeners) { mPanelSlideListeners.remove(listener) }
    }

    private fun setDragView(dragView: View?) {
        mDragView?.setOnClickListener(null)
        mDragView = dragView
        mDragView?.let {
            it.isClickable = true
            it.isFocusable = false
            it.isFocusableInTouchMode = false
            it.setOnClickListener(OnClickListener {
                if (!isEnabled || !isTouchEnabled) return@OnClickListener
                panelState =
                    if (mSlideState != PanelState.EXPANDED && mSlideState != PanelState.ANCHORED) {
                        if (mAnchorPoint < 1.0f) {
                            PanelState.ANCHORED
                        } else {
                            PanelState.EXPANDED
                        }
                    } else {
                        PanelState.COLLAPSED
                    }
            })
        }
    }

    private fun setScrollableView(scrollableView: View?) {
        mScrollableView = scrollableView
    }

    private fun dispatchOnPanelSlide(panel: View?) {
        synchronized(mPanelSlideListeners) {
            for (l in mPanelSlideListeners) {
                l.onPanelSlide(panel, mSlideOffset)
            }
        }
    }

    private fun dispatchOnPanelStateChanged(
        panel: View?,
        previousState: PanelState?,
        newState: PanelState?
    ) {
        synchronized(mPanelSlideListeners) {
            for (l in mPanelSlideListeners) {
                l.onPanelStateChanged(panel, previousState, newState)
            }
        }
    }

    fun updateObscuredViewVisibility() {
        if (childCount == 0) {
            return
        }
        val leftBound = paddingLeft
        val rightBound = width - paddingRight
        val topBound = paddingTop
        val bottomBound = height - paddingBottom
        mSlideableView?.let {
            var left = 0
            var right = 0
            var top = 0
            var bottom = 0
            if (hasOpaqueBackground(it)) {
                left = it.left
                right = it.right
                top = it.top
                bottom = it.bottom
            }
            val child = getChildAt(0)
            val clampedChildLeft = max(leftBound, child.left)
            val clampedChildTop = max(topBound, child.top)
            val clampedChildRight = min(rightBound, child.right)
            val clampedChildBottom = min(bottomBound, child.bottom)
            val vis: Int =
                if (clampedChildLeft >= left && clampedChildTop >= top && clampedChildRight <= right && clampedChildBottom <= bottom) {
                    INVISIBLE
                } else {
                    VISIBLE
                }
            child.visibility = vis
        }
    }

    fun setAllChildrenVisible() {
        var i = 0
        val childCount = childCount
        while (i < childCount) {
            val child = getChildAt(i)
            if (child.visibility == INVISIBLE) {
                child.visibility = VISIBLE
            }
            i++
        }
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        mFirstLayout = true
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        mFirstLayout = true
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val widthMode = MeasureSpec.getMode(widthMeasureSpec)
        val widthSize = MeasureSpec.getSize(widthMeasureSpec)
        val heightMode = MeasureSpec.getMode(heightMeasureSpec)
        val heightSize = MeasureSpec.getSize(heightMeasureSpec)
        check(!(widthMode != MeasureSpec.EXACTLY && widthMode != MeasureSpec.AT_MOST)) { "Width must have an exact value or MATCH_PARENT" }
        check(!(heightMode != MeasureSpec.EXACTLY && heightMode != MeasureSpec.AT_MOST)) { "Height must have an exact value or MATCH_PARENT" }
        val childCount = childCount
        check(childCount == 2) { "Sliding up panel layout must have exactly 2 children!" }
        mMainView = getChildAt(0)
        mSlideableView = getChildAt(1)
        if (mDragView == null) {
            setDragView(mSlideableView)
        }

        // If the sliding panel is not visible, then put the whole view in the hidden state
        if (mSlideableView?.visibility != VISIBLE) {
            mSlideState = PanelState.HIDDEN
        }
        val layoutHeight = heightSize - paddingTop - paddingBottom
        val layoutWidth = widthSize - paddingLeft - paddingRight

        // First pass. Measure based on child LayoutParams width/height.
        for (i in 0 until childCount) {
            val child = getChildAt(i)
            val lp = child.layoutParams as LayoutParams

            // We always measure the sliding panel in order to know it's height (needed for show panel)
            if (child.visibility == GONE && i == 0) {
                continue
            }
            var height = layoutHeight
            var width = layoutWidth
            if (child === mMainView) {
                if (!isOverlayed && mSlideState != PanelState.HIDDEN) {
                    height -= mPanelHeight
                }
                width -= lp.leftMargin + lp.rightMargin
            } else if (child === mSlideableView) {
                // The slideable view should be aware of its top margin.
                // See https://github.com/umano/AndroidSlidingUpPanel/issues/412.
                height -= lp.topMargin
            }
            val childWidthSpec: Int = when (lp.width) {
                ViewGroup.LayoutParams.WRAP_CONTENT -> {
                    MeasureSpec.makeMeasureSpec(width, MeasureSpec.AT_MOST)
                }

                ViewGroup.LayoutParams.MATCH_PARENT -> {
                    MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY)
                }

                else -> {
                    MeasureSpec.makeMeasureSpec(lp.width, MeasureSpec.EXACTLY)
                }
            }
            var childHeightSpec: Int
            if (lp.height == ViewGroup.LayoutParams.WRAP_CONTENT) {
                childHeightSpec = MeasureSpec.makeMeasureSpec(height, MeasureSpec.AT_MOST)
            } else {
                // Modify the height based on the weight.
                if (lp.weight > 0 && lp.weight < 1) {
                    height = (height * lp.weight).toInt()
                } else if (lp.height != ViewGroup.LayoutParams.MATCH_PARENT) {
                    height = lp.height
                }
                childHeightSpec = MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY)
            }
            child.measure(childWidthSpec, childHeightSpec)
            if (child === mSlideableView) {
                mSlideableView?.let {
                    mSlideRange = it.measuredHeight - mPanelHeight
                }
            }
        }
        setMeasuredDimension(widthSize, heightSize)
    }

    override fun onLayout(changed: Boolean, l: Int, t: Int, r: Int, b: Int) {
        val paddingLeft = paddingLeft
        val paddingTop = paddingTop
        val childCount = childCount
        if (mFirstLayout) {
            mSlideOffset = when (mSlideState) {
                PanelState.EXPANDED -> 1.0f
                PanelState.ANCHORED -> mAnchorPoint
                PanelState.HIDDEN -> {
                    val newTop =
                        computePanelTopPosition(0.0f) + if (mIsSlidingUp) +mPanelHeight else -mPanelHeight
                    computeSlideOffset(newTop)
                }

                else -> 0f
            }
        }
        for (i in 0 until childCount) {
            val child = getChildAt(i)
            val lp = child.layoutParams as LayoutParams

            // Always layout the sliding view on the first layout
            if (child.visibility == GONE && (i == 0 || mFirstLayout)) {
                continue
            }
            val childHeight = child.measuredHeight
            var childTop = paddingTop
            if (child === mSlideableView) {
                childTop = computePanelTopPosition(mSlideOffset)
            }
            if (!mIsSlidingUp) {
                if (child === mMainView && !isOverlayed) {
                    childTop =
                        computePanelTopPosition(mSlideOffset) + (mSlideableView?.measuredHeight
                            ?: 0)
                }
            }
            val childBottom = childTop + childHeight
            val childLeft = paddingLeft + lp.leftMargin
            val childRight = childLeft + child.measuredWidth
            child.layout(childLeft, childTop, childRight, childBottom)
        }
        if (mFirstLayout) {
            updateObscuredViewVisibility()
        }
        applyParallaxForCurrentSlideOffset()
        mFirstLayout = false
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        // Recalculate sliding panes and their details
        if (h != oldh) {
            mFirstLayout = true
        }
    }

    override fun onInterceptTouchEvent(ev: MotionEvent): Boolean {
        // If the scrollable view is handling touch, never intercept
        if (mIsScrollableViewHandlingTouch || !isTouchEnabled) {
            mDragHelper?.abort()
            return false
        }
        val action = ev.action
        val x = ev.x
        val y = ev.y
        val adx = abs(x - mInitialMotionX)
        val ady = abs(y - mInitialMotionY)
        val dragSlop = mDragHelper?.touchSlop
        when (action) {
            MotionEvent.ACTION_DOWN -> {
                mIsUnableToDrag = false
                mInitialMotionX = x
                mInitialMotionY = y
                if (!isViewUnder(mDragView, x.toInt(), y.toInt())) {
                    mDragHelper?.cancel()
                    mIsUnableToDrag = true
                    return false
                }
            }

            MotionEvent.ACTION_MOVE -> {
                dragSlop?.let {
                    if (ady > it && adx > ady) {
                        mDragHelper?.cancel()
                        mIsUnableToDrag = true
                        return false
                    }
                }
            }

            MotionEvent.ACTION_CANCEL, MotionEvent.ACTION_UP -> {
                // If the dragView is still dragging when we get here, we need to call processTouchEvent
                // so that the view is settled
                // Added to make scrollable views work (tokudu)
                if (mDragHelper?.isDragging == true) {
                    mDragHelper?.processTouchEvent(ev)
                    return true
                }
                // Check if this was a click on the faded part of the screen, and fire off the listener if there is one.
                dragSlop?.let {
                    if (ady <= it && adx <= it && mSlideOffset > 0 && !isViewUnder(
                            mSlideableView,
                            mInitialMotionX.toInt(),
                            mInitialMotionY.toInt()
                        ) && mFadeOnClickListener != null
                    ) {
                        playSoundEffect(SoundEffectConstants.CLICK)
                        mFadeOnClickListener?.onClick(this)
                        return true
                    }
                }
            }
        }
        return mDragHelper?.shouldInterceptTouchEvent(ev) == true
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(ev: MotionEvent): Boolean {
        return if (!isEnabled || !isTouchEnabled) {
            super.onTouchEvent(ev)
        } else try {
            mDragHelper?.processTouchEvent(ev)
            true
        } catch (ex: Exception) {
            // Ignore the pointer out of range exception
            false
        }
    }

    override fun dispatchTouchEvent(ev: MotionEvent): Boolean {
        val action = ev.action
        if (!isEnabled || !isTouchEnabled || mIsUnableToDrag && action != MotionEvent.ACTION_DOWN) {
            mDragHelper?.abort()
            return super.dispatchTouchEvent(ev)
        }
        val x = ev.x
        val y = ev.y
        if (action == MotionEvent.ACTION_DOWN) {
            mIsScrollableViewHandlingTouch = false
            mPrevMotionX = x
            mPrevMotionY = y
        } else if (action == MotionEvent.ACTION_MOVE) {
            val dx = x - mPrevMotionX
            val dy = y - mPrevMotionY
            mPrevMotionX = x
            mPrevMotionY = y
            if (abs(dx) > abs(dy)) {
                // Scrolling horizontally, so ignore
                return super.dispatchTouchEvent(ev)
            }

            // If the scroll view isn't under the touch, pass the
            // event along to the dragView.
            if (!isViewUnder(mScrollableView, mInitialMotionX.toInt(), mInitialMotionY.toInt())) {
                return super.dispatchTouchEvent(ev)
            }

            // Which direction (up or down) is the drag moving?
            if (dy * (if (mIsSlidingUp) 1 else -1) > 0) { // Collapsing
                // Is the child less than fully scrolled?
                // Then let the child handle it.
                if (mScrollableViewHelper.getScrollableViewScrollPosition(
                        mScrollableView,
                        mIsSlidingUp
                    ) > 0
                ) {
                    mIsScrollableViewHandlingTouch = true
                    return super.dispatchTouchEvent(ev)
                }

                // Was the child handling the touch previously?
                // Then we need to rejigger things so that the
                // drag panel gets a proper down event.
                if (mIsScrollableViewHandlingTouch) {
                    // Send an 'UP' event to the child.
                    val up = MotionEvent.obtain(ev)
                    up.action = MotionEvent.ACTION_CANCEL
                    super.dispatchTouchEvent(up)
                    up.recycle()

                    // Send a 'DOWN' event to the panel. (We'll cheat
                    // and hijack this one)
                    ev.action = MotionEvent.ACTION_DOWN
                }
                mIsScrollableViewHandlingTouch = false
                return onTouchEvent(ev)
            } else if (dy * (if (mIsSlidingUp) 1 else -1) < 0) { // Expanding
                // Is the panel less than fully expanded?
                // Then we'll handle the drag here.
                if (mSlideOffset < 1.0f) {
                    mIsScrollableViewHandlingTouch = false
                    return onTouchEvent(ev)
                }

                // Was the panel handling the touch previously?
                // Then we need to rejigger things so that the
                // child gets a proper down event.
                if (!mIsScrollableViewHandlingTouch && mDragHelper?.isDragging == true) {
                    mDragHelper?.cancel()
                    ev.action = MotionEvent.ACTION_DOWN
                }
                mIsScrollableViewHandlingTouch = true
                return super.dispatchTouchEvent(ev)
            }
        } else if (action == MotionEvent.ACTION_UP) {
            // If the scrollable view was handling the touch and we receive an up
            // we want to clear any previous dragging state so we don't intercept a touch stream accidentally
            if (mIsScrollableViewHandlingTouch) {
                mDragHelper?.setDragState(ViewDragHelper.STATE_IDLE)
            }
        }

        // In all other cases, just let the default behavior take over.
        return super.dispatchTouchEvent(ev)
    }

    private fun isViewUnder(view: View?, x: Int, y: Int): Boolean {
        if (view == null) return false
        val viewLocation = IntArray(2)
        view.getLocationOnScreen(viewLocation)
        val parentLocation = IntArray(2)
        getLocationOnScreen(parentLocation)
        val screenX = parentLocation[0] + x
        val screenY = parentLocation[1] + y
        return screenX >= viewLocation[0] && screenX < viewLocation[0] + view.width && screenY >= viewLocation[1] && screenY < viewLocation[1] + view.height
    }

    /*
     * Computes the top position of the panel based on the slide offset.
     */
    private fun computePanelTopPosition(slideOffset: Float): Int {
        val slidingViewHeight = mSlideableView?.measuredHeight ?: 0
        val slidePixelOffset = (slideOffset * mSlideRange).toInt()
        // Compute the top of the panel if its collapsed
        return if (mIsSlidingUp) measuredHeight - paddingBottom - mPanelHeight - slidePixelOffset else paddingTop - slidingViewHeight + mPanelHeight + slidePixelOffset
    }

    /*
     * Computes the slide offset based on the top position of the panel
     */
    private fun computeSlideOffset(topPosition: Int): Float {
        // Compute the panel top position if the panel is collapsed (offset 0)
        val topBoundCollapsed = computePanelTopPosition(0f)

        // Determine the new slide offset based on the collapsed top position and the new required
        // top position
        return if (mIsSlidingUp) (topBoundCollapsed - topPosition).toFloat() / mSlideRange else (topPosition - topBoundCollapsed).toFloat() / mSlideRange
    }

    private var panelState: PanelState?
        get() = mSlideState
        set(value) {
            mSlideState = value
        }

    private fun setPanelStateInternal(state: PanelState) {
        if (mSlideState == state) return
        val oldState = mSlideState
        mSlideState = state
        dispatchOnPanelStateChanged(this, oldState, state)
    }

    private fun applyParallaxForCurrentSlideOffset() {
        if (mParallaxOffset > 0) {
            val mainViewOffset = currentParallaxOffset
            mMainView?.translationY = mainViewOffset.toFloat()
        }
    }

    private fun onPanelDragged(newTop: Int) {
        if (mSlideState != PanelState.DRAGGING) {
            mLastNotDraggingSlideState = mSlideState
        }
        setPanelStateInternal(PanelState.DRAGGING)
        // Recompute the slide offset based on the new top position
        mSlideOffset = computeSlideOffset(newTop)
        applyParallaxForCurrentSlideOffset()
        // Dispatch the slide event
        dispatchOnPanelSlide(mSlideableView)
        // If the slide offset is negative, and overlay is not on, we need to increase the
        // height of the main content
        val lp = mMainView?.layoutParams as LayoutParams
        val defaultHeight = height - paddingBottom - paddingTop - mPanelHeight
        if (mSlideOffset <= 0 && !isOverlayed) {
            // expand the main view
            lp.height =
                if (mIsSlidingUp) newTop - paddingBottom else height - paddingBottom - (mSlideableView?.measuredHeight
                    ?: 0) - newTop
            if (lp.height == defaultHeight) {
                lp.height = ViewGroup.LayoutParams.MATCH_PARENT
            }
            mMainView?.requestLayout()
        } else if (lp.height != ViewGroup.LayoutParams.MATCH_PARENT && !isOverlayed) {
            lp.height = ViewGroup.LayoutParams.MATCH_PARENT
            mMainView?.requestLayout()
        }
    }

    override fun drawChild(canvas: Canvas, child: View, drawingTime: Long): Boolean {
        val result: Boolean
        val save = canvas.save()
        if (mSlideableView != null && mSlideableView !== child) { // if main view
            // Clip against the slider; no sense drawing what will immediately be covered,
            // Unless the panel is set to overlay content
            canvas.getClipBounds(mTmpRect)
            if (!isOverlayed) {
                mSlideableView?.let {
                    if (mIsSlidingUp) {
                        mTmpRect.bottom = min(mTmpRect.bottom, it.top)
                    } else {
                        mTmpRect.top = max(mTmpRect.top, it.bottom)
                    }
                }
            }
            if (isClipPanel) {
                canvas.clipRect(mTmpRect)
            }
            result = super.drawChild(canvas, child, drawingTime)
            if (mCoveredFadeColor != 0 && mSlideOffset > 0) {
                val baseAlpha = mCoveredFadeColor and -0x1000000 ushr 24
                val imag = (baseAlpha * mSlideOffset).toInt()
                val color = imag shl 24 or (mCoveredFadeColor and 0xffffff)
                mCoveredFadePaint.color = color
                canvas.drawRect(mTmpRect, mCoveredFadePaint)
            }
        } else {
            result = super.drawChild(canvas, child, drawingTime)
        }
        canvas.restoreToCount(save)
        return result
    }

    override fun computeScroll() {
        if (mDragHelper != null && mDragHelper?.continueSettling(true) == true) {
            if (!isEnabled) {
                mDragHelper?.abort()
                return
            }
        }
    }

    override fun draw(c: Canvas) {
        super.draw(c)

        mSlideableView?.let {
            // draw the shadow
            if (mShadowDrawable != null) {
                val right = it.right
                val top: Int
                val bottom: Int
                if (mIsSlidingUp) {
                    top = it.top - mShadowHeight
                    bottom = it.top
                } else {
                    top = it.bottom
                    bottom = it.bottom + mShadowHeight
                }
                val left = it.left
                mShadowDrawable?.setBounds(left, top, right, bottom)
                mShadowDrawable?.draw(c)
            }
        }
    }

    override fun generateDefaultLayoutParams(): ViewGroup.LayoutParams {
        return LayoutParams()
    }

    override fun generateLayoutParams(p: ViewGroup.LayoutParams): ViewGroup.LayoutParams {
        return if (p is MarginLayoutParams) LayoutParams(p) else LayoutParams(p)
    }

    override fun checkLayoutParams(p: ViewGroup.LayoutParams): Boolean {
        return p is LayoutParams && super.checkLayoutParams(p)
    }

    override fun generateLayoutParams(attrs: AttributeSet): ViewGroup.LayoutParams {
        return LayoutParams(context, attrs)
    }

    public override fun onSaveInstanceState(): Parcelable {
        val bundle = Bundle()
        bundle.putParcelable("superState", super.onSaveInstanceState())
        bundle.putSerializable(
            SLIDING_STATE,
            if (mSlideState != PanelState.DRAGGING) mSlideState else mLastNotDraggingSlideState
        )
        return bundle
    }

    @Suppress("DEPRECATION")
    public override fun onRestoreInstanceState(state: Parcelable) {
        var newState: Parcelable? = state
        if (newState is Bundle) {
            val bundle = newState
            mSlideState = bundle.getSerializable(SLIDING_STATE) as PanelState?
            mSlideState = if (mSlideState == null) DEFAULT_SLIDE_STATE else mSlideState
            newState = bundle.getParcelable("superState")
        }
        super.onRestoreInstanceState(newState)
    }

    private inner class DragHelperCallback : ViewDragHelper.Callback() {
        override fun tryCaptureView(child: View?, pointerId: Int): Boolean {
            return !mIsUnableToDrag && child === mSlideableView
        }

        override fun onViewDragStateChanged(state: Int) {
            if (mDragHelper != null && mDragHelper?.viewDragState == ViewDragHelper.STATE_IDLE) {
                mSlideableView?.let {
                    mSlideOffset = computeSlideOffset(it.top)
                    applyParallaxForCurrentSlideOffset()
                    if (mSlideOffset == 1f) {
                        updateObscuredViewVisibility()
                        setPanelStateInternal(PanelState.EXPANDED)
                    } else if (mSlideOffset == 0f) {
                        setPanelStateInternal(PanelState.COLLAPSED)
                    } else if (mSlideOffset < 0) {
                        setPanelStateInternal(PanelState.HIDDEN)
                        it.visibility = INVISIBLE
                    } else {
                        updateObscuredViewVisibility()
                        setPanelStateInternal(PanelState.ANCHORED)
                    }
                }
            }
        }

        override fun onViewCaptured(capturedChild: View?, activePointerId: Int) {
            setAllChildrenVisible()
        }

        override fun onViewPositionChanged(
            changedView: View?,
            left: Int,
            top: Int,
            dx: Int,
            dy: Int
        ) {
            onPanelDragged(top)
            invalidate()
        }

        override fun onViewReleased(releasedChild: View?, xVel: Float, yVel: Float) {
            // direction is always positive if we are sliding in the expanded direction
            val direction = if (mIsSlidingUp) -yVel else yVel
            val target = if (direction > 0 && mSlideOffset <= mAnchorPoint) {
                // swipe up -> expand and stop at anchor point
                computePanelTopPosition(mAnchorPoint)
            } else if (direction < 0 && mSlideOffset >= mAnchorPoint) {
                // swipe down -> collapse and stop at anchor point
                computePanelTopPosition(mAnchorPoint)
            } else if (mSlideOffset >= (1f + mAnchorPoint) / 2) {
                // zero velocity, and far enough from anchor point => expand to the top
                computePanelTopPosition(1.0f)
            } else if (mSlideOffset >= mAnchorPoint / 2) {
                // zero velocity, and close enough to anchor point => go to anchor
                computePanelTopPosition(mAnchorPoint)
            } else {
                // settle at the bottom
                computePanelTopPosition(0.0f)
            }
            if (mDragHelper != null) {
                releasedChild?.left?.let { mDragHelper?.settleCapturedViewAt(it, target) }
            }
            invalidate()
        }

        override fun getViewVerticalDragRange(child: View?): Int {
            return mSlideRange
        }

        override fun clampViewPositionVertical(child: View?, top: Int, dy: Int): Int {
            val collapsedTop = computePanelTopPosition(0f)
            val expandedTop = computePanelTopPosition(1.0f)
            return if (mIsSlidingUp) {
                min(max(top, expandedTop), collapsedTop)
            } else {
                min(max(top, collapsedTop), expandedTop)
            }
        }
    }

    class LayoutParams : MarginLayoutParams {
        var weight = 0f

        constructor() : super(MATCH_PARENT, MATCH_PARENT)

        constructor(source: ViewGroup.LayoutParams?) : super(source)
        constructor(source: MarginLayoutParams?) : super(source)
        constructor(c: Context, attrs: AttributeSet?) : super(c, attrs) {
            val ta = c.obtainStyledAttributes(attrs, ATTRS)
            weight = ta.getFloat(0, 0f)
            ta.recycle()
        }

        companion object {
            private val ATTRS = intArrayOf(
                android.R.attr.layout_weight
            )
        }
    }

    companion object {
        /**
         * Default peeking out panel height
         */
        const val DEFAULT_PANEL_HEIGHT = 68 // dp;

        /**
         * Default initial state for the component
         */
        private val DEFAULT_SLIDE_STATE = PanelState.COLLAPSED

        /**
         * Default height of the shadow above the peeking out panel
         */
        private const val DEFAULT_SHADOW_HEIGHT = 4 // dp;

        /**
         * If no fade color is given by default it will fade to 80% gray.
         */
        private const val DEFAULT_FADE_COLOR = -0x67000000

        /**
         * Default Minimum velocity that will be detected as a fling
         */
        private const val DEFAULT_MIN_FLING_VELOCITY = 400 // dips per second

        /**
         * Default is set to false because that is how it was written
         */
        private const val DEFAULT_OVERLAY_FLAG = false

        /**
         * Default is set to true for clip panel for performance reasons
         */
        private const val DEFAULT_CLIP_PANEL_FLAG = true

        /**
         * Default attributes for layout
         */
        private val DEFAULT_ATTRS = intArrayOf(
            android.R.attr.gravity
        )

        /**
         * Tag for the sliding state stored inside the bundle
         */
        const val SLIDING_STATE = "sliding_state"

        /**
         * Default parallax length of the main view
         */
        private const val DEFAULT_PARALLAX_OFFSET = 0

        @Suppress("DEPRECATION")
        private fun hasOpaqueBackground(v: View): Boolean {
            val bg = v.background
            return bg != null && bg.opacity == PixelFormat.OPAQUE
        }
    }
}
