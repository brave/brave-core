/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.slidingpanel

import android.content.Context
import android.view.MotionEvent
import android.view.VelocityTracker
import android.view.View
import android.view.ViewConfiguration
import android.view.ViewGroup
import android.view.animation.Interpolator
import android.widget.OverScroller
import androidx.core.view.VelocityTrackerCompat
import java.util.Arrays
import kotlin.math.abs
import kotlin.math.min
import kotlin.math.roundToInt
import kotlin.math.sin


/**
 * ViewDragHelper is a utility class for writing custom ViewGroups. It offers a number
 * of useful operations and state tracking for allowing a user to drag and reposition
 * views within their parent ViewGroup.
 */
class ViewDragHelper private constructor(
    context: Context,
    forParent: ViewGroup?,
    interpolator: Interpolator?,
    cb: Callback?
) {
    /**
     * Retrieve the current drag state of this helper. This will return one of
     * [.STATE_IDLE], [.STATE_DRAGGING] or [.STATE_SETTLING].
     * @return The current drag state
     */
    // Current drag state; idle, dragging or settling
    var viewDragState = 0
        private set

    /**
     * @return The minimum distance in pixels that the user must travel to initiate a drag
     */
    // Distance to travel before a drag may begin
    var touchSlop: Int
        private set

    /**
     * @return The ID of the pointer currently dragging the captured view,
     * or [.INVALID_POINTER].
     */
    // Last known position/pointer tracking
    private var activePointerId = INVALID_POINTER
    private var mInitialMotionX: FloatArray? = null
    private var mInitialMotionY: FloatArray? = null
    private var mLastMotionX: FloatArray? = null
    private var mLastMotionY: FloatArray? = null
    private lateinit var mInitialEdgesTouched: IntArray
    private lateinit var mEdgeDragsInProgress: IntArray
    private lateinit var mEdgeDragsLocked: IntArray
    private var mPointersDown = 0
    private var mVelocityTracker: VelocityTracker? = null
    private val mMaxVelocity: Float

    /**
     * Return the currently configured minimum velocity. Any flings with a magnitude less
     * than this value in pixels per second. Callback methods accepting a velocity will receive
     * zero as a velocity value if the real detected velocity was below this threshold.
     *
     * @return the minimum velocity that will be detected
     */
    var minVelocity: Float

    /**
     * Return the size of an edge. This is the range in pixels along the edges of this view
     * that will actively detect edge touches or drags if edge tracking is enabled.
     *
     * @return The size of an edge in pixels
     * @see .setEdgeTrackingEnabled
     */
    private val edgeSize: Int
    private var mTrackingEdges = 0
    private val mScroller: OverScroller
    private val mCallback: Callback

    /**
     * @return The currently captured view, or null if no view has been captured.
     */
    private var capturedView: View? = null
    private var mReleaseInProgress = false
    private val mParentView: ViewGroup

    /**
     * A Callback is used as a communication channel with the ViewDragHelper back to the
     * parent view using it. `on*`methods are invoked on significant events and several
     * accessor methods are expected to provide the ViewDragHelper with more information
     * about the state of the parent view upon request. The callback also makes decisions
     * governing the range and draggability of child views.
     */
    abstract class Callback {
        /**
         * Called when the drag state changes. See the `STATE_*` constants
         * for more information.
         *
         * @param state The new drag state
         *
         * @see .STATE_IDLE
         *
         * @see .STATE_DRAGGING
         *
         * @see .STATE_SETTLING
         */
        open fun onViewDragStateChanged(state: Int) {}

        /**
         * Called when the captured view's position changes as the result of a drag or settle.
         *
         * @param changedView View whose position changed
         * @param left New X coordinate of the left edge of the view
         * @param top New Y coordinate of the top edge of the view
         * @param dx Change in X position from the last call
         * @param dy Change in Y position from the last call
         */
        open fun onViewPositionChanged(changedView: View?, left: Int, top: Int, dx: Int, dy: Int) {}

        /**
         * Called when a child view is captured for dragging or settling. The ID of the pointer
         * currently dragging the captured view is supplied. If activePointerId is
         * identified as [.INVALID_POINTER] the capture is programmatic instead of
         * pointer-initiated.
         *
         * @param capturedChild Child view that was captured
         * @param activePointerId Pointer id tracking the child capture
         */
        open fun onViewCaptured(capturedChild: View?, activePointerId: Int) {}

        /**
         * Called when the child view is no longer being actively dragged.
         * The fling velocity is also supplied, if relevant. The velocity values may
         * be clamped to system minimums or maximums.
         *
         *
         * Calling code may decide to fling or otherwise release the view to let it
         * settle into place. It should do so using [.settleCapturedViewAt]
         * or [.flingCapturedView]. If the Callback invokes
         * one of these methods, the ViewDragHelper will enter [.STATE_SETTLING]
         * and the view capture will not fully end until it comes to a complete stop.
         * If neither of these methods is invoked before `onViewReleased` returns,
         * the view will stop in place and the ViewDragHelper will return to
         * [.STATE_IDLE].
         *
         * @param releasedChild The captured child view now being released
         * @param xVel X velocity of the pointer as it left the screen in pixels per second.
         * @param yVel Y velocity of the pointer as it left the screen in pixels per second.
         */
        open fun onViewReleased(releasedChild: View?, xVel: Float, yVel: Float) {}

        /**
         * Called to determine the Z-order of child views.
         *
         * @param index the ordered position to query for
         * @return index of the view that should be ordered at position `index`
         */
        fun getOrderedChildIndex(index: Int): Int {
            return index
        }

        /**
         * Return the magnitude of a draggable child view's vertical range of motion in pixels.
         * This method should return 0 for views that cannot move vertically.
         *
         * @param child Child view to check
         * @return range of vertical motion in pixels
         */
        open fun getViewVerticalDragRange(child: View?): Int {
            return 0
        }

        /**
         * Called when the user's input indicates that they want to capture the given child view
         * with the pointer indicated by pointerId. The callback should return true if the user
         * is permitted to drag the given view with the indicated pointer.
         *
         *
         * ViewDragHelper may call this method multiple times for the same view even if
         * the view is already captured; this indicates that a new pointer is trying to take
         * control of the view.
         *
         *
         * If this method returns true, a call to [.onViewCaptured]
         * will follow if the capture is successful.
         *
         * @param child Child the user is attempting to capture
         * @param pointerId ID of the pointer attempting the capture
         * @return true if capture should be allowed, false otherwise
         */
        abstract fun tryCaptureView(child: View?, pointerId: Int): Boolean

        /**
         * Restrict the motion of the dragged child view along the vertical axis.
         * The default implementation does not allow vertical motion; the extending
         * class must override this method and provide the desired clamping.
         *
         *
         * @param child Child view being dragged
         * @param top Attempted motion along the Y axis
         * @param dy Proposed change in position for top
         * @return The new clamped position for top
         */
        open fun clampViewPositionVertical(child: View?, top: Int, dy: Int): Int {
            return 0
        }
    }

    private val mSetIdleRunnable: Runnable = Runnable { setDragState(STATE_IDLE) }

    init {
        if (forParent == null) {
            throw IllegalArgumentException("Parent view may not be null")
        }
        if (cb == null) {
            throw IllegalArgumentException("Callback may not be null")
        }
        mParentView = forParent
        mCallback = cb
        val vc = ViewConfiguration.get(context)
        val density = context.resources.displayMetrics.density
        edgeSize = (EDGE_SIZE * density + 0.5f).toInt()
        touchSlop = vc.scaledTouchSlop
        mMaxVelocity = vc.scaledMaximumFlingVelocity.toFloat()
        minVelocity = vc.scaledMinimumFlingVelocity.toFloat()
        mScroller = OverScroller(context, interpolator ?: sInterpolator)
    }

    /**
     * Capture a specific child view for dragging within the parent. The callback will be notified
     * but [Callback.tryCaptureView] will not be asked permission to
     * capture this view.
     *
     * @param childView Child view to capture
     * @param activePointerId ID of the pointer that is dragging the captured child view
     */
    private fun captureChildView(childView: View, activePointerId: Int) {
        if (childView.parent !== mParentView) {
            throw IllegalArgumentException(
                "captureChildView: parameter must be a descendant " +
                        "of the ViewDragHelper's tracked parent view (" + mParentView + ")"
            )
        }
        capturedView = childView
        this.activePointerId = activePointerId
        mCallback.onViewCaptured(childView, activePointerId)
        setDragState(STATE_DRAGGING)
    }

    /**
     * The result of a call to this method is equivalent to
     * [.processTouchEvent] receiving an ACTION_CANCEL event.
     */
    fun cancel() {
        activePointerId = INVALID_POINTER
        clearMotionHistory()
        if (mVelocityTracker != null) {
            mVelocityTracker?.recycle()
            mVelocityTracker = null
        }
    }

    /**
     * [.cancel], but also abort all motion in progress and snap to the end of any
     * animation.
     */
    fun abort() {
        cancel()
        if (viewDragState == STATE_SETTLING) {
            val oldX = mScroller.currX
            val oldY = mScroller.currY
            mScroller.abortAnimation()
            val newX = mScroller.currX
            val newY = mScroller.currY
            mCallback.onViewPositionChanged(capturedView, newX, newY, newX - oldX, newY - oldY)
        }
        setDragState(STATE_IDLE)
    }

    /**
     * Animate the view `child` to the given (left, top) position.
     * If this method returns true, the caller should invoke [.continueSettling]
     * on each subsequent frame to continue the motion until it returns false. If this method
     * returns false there is no further work to do to complete the movement.
     *
     *
     * This operation does not count as a capture event, though [.getCapturedView]
     * will still report the sliding view while the slide is in progress.
     *
     * @param child Child view to capture and animate
     * @param finalLeft Final left position of child
     * @param finalTop Final top position of child
     * @return true if animation should continue through [.continueSettling] calls
     */
    fun smoothSlideViewTo(child: View?, finalLeft: Int, finalTop: Int): Boolean {
        capturedView = child
        activePointerId = INVALID_POINTER
        return forceSettleCapturedViewAt(finalLeft, finalTop, 0, 0)
    }

    /**
     * Settle the captured view at the given (left, top) position.
     * The appropriate velocity from prior motion will be taken into account.
     * If this method returns true, the caller should invoke [.continueSettling]
     * on each subsequent frame to continue the motion until it returns false. If this method
     * returns false there is no further work to do to complete the movement.
     *
     * @param finalLeft Settled left edge position for the captured view
     * @param finalTop Settled top edge position for the captured view
     * @return true if animation should continue through [.continueSettling] calls
     */
    @Suppress("DEPRECATION")
    fun settleCapturedViewAt(finalLeft: Int, finalTop: Int): Boolean {
        if (!mReleaseInProgress) {
            throw IllegalStateException(
                "Cannot settleCapturedViewAt outside of a call to " +
                        "Callback#onViewReleased"
            )
        }
        return forceSettleCapturedViewAt(
            finalLeft, finalTop, VelocityTrackerCompat.getXVelocity(
                mVelocityTracker,
                activePointerId
            ).toInt(), VelocityTrackerCompat.getYVelocity(mVelocityTracker, activePointerId).toInt()
        )
    }

    /**
     * Settle the captured view at the given (left, top) position.
     *
     * @param finalLeft Target left position for the captured view
     * @param finalTop Target top position for the captured view
     * @param xVelocity Horizontal velocity
     * @param yVelocity Vertical velocity
     * @return true if animation should continue through [.continueSettling] calls
     */
    private fun forceSettleCapturedViewAt(
        finalLeft: Int,
        finalTop: Int,
        xVelocity: Int,
        yVelocity: Int
    ): Boolean {
        val startLeft = capturedView?.left
        val startTop = capturedView?.top
        val dx = finalLeft - (startLeft ?: 0)
        val dy = finalTop - (startTop ?: 0)
        if (dx == 0 && dy == 0) {
            // Nothing to do. Send callbacks, be done.
            mScroller.abortAnimation()
            setDragState(STATE_IDLE)
            return false
        }
        val duration = computeSettleDuration(capturedView, dx, dy, xVelocity, yVelocity)
        mScroller.startScroll((startLeft ?: 0), (startTop ?: 0), dx, dy, duration)
        setDragState(STATE_SETTLING)
        return true
    }

    private fun computeSettleDuration(
        child: View?,
        dx: Int,
        dy: Int,
        xVelocity: Int,
        yVelocity: Int
    ): Int {
        var xVel = xVelocity
        var yVel = yVelocity
        xVel = clampMag(xVel, minVelocity.toInt(), mMaxVelocity.toInt())
        yVel = clampMag(yVel, minVelocity.toInt(), mMaxVelocity.toInt())
        val absDx = abs(dx)
        val absDy = abs(dy)
        val absXVel = abs(xVel)
        val absYVel = abs(yVel)
        val addedVel = absXVel + absYVel
        val addedDistance = absDx + absDy
        val xWeight =
            if (xVel != 0) absXVel.toFloat() / addedVel else absDx.toFloat() / addedDistance
        val yWeight =
            if (yVel != 0) absYVel.toFloat() / addedVel else absDy.toFloat() / addedDistance
        val xDuration = computeAxisDuration(dx, xVel, 0)
        val yDuration = computeAxisDuration(dy, yVel, mCallback.getViewVerticalDragRange(child))
        return (xDuration * xWeight + yDuration * yWeight).toInt()
    }

    private fun computeAxisDuration(delta: Int, velocity: Int, motionRange: Int): Int {
        var newVelocity = velocity
        if (delta == 0) {
            return 0
        }
        val width = mParentView.width
        val halfWidth = width / 2
        val distanceRatio = min(1f, abs(delta).toFloat() / width)
        val distance = halfWidth + halfWidth *
                distanceInfluenceForSnapDuration(distanceRatio)
        newVelocity = abs(newVelocity)
        val duration: Int = if (newVelocity > 0) {
            4 * (1000 * abs(distance / newVelocity)).roundToInt()
        } else {
            val range = abs(delta).toFloat() / motionRange
            ((range + 1) * BASE_SETTLE_DURATION).toInt()
        }
        return min(duration, MAX_SETTLE_DURATION)
    }

    /**
     * Clamp the magnitude of value for absMin and absMax.
     * If the value is below the minimum, it will be clamped to zero.
     * If the value is above the maximum, it will be clamped to the maximum.
     *
     * @param value Value to clamp
     * @param absMin Absolute value of the minimum significant value to return
     * @param absMax Absolute value of the maximum value to return
     * @return The clamped value with the same sign as `value`
     */
    private fun clampMag(value: Int, absMin: Int, absMax: Int): Int {
        val absValue = abs(value)
        if (absValue < absMin) return 0
        return if (absValue > absMax) if (value > 0) absMax else -absMax else value
    }

    /**
     * Clamp the magnitude of value for absMin and absMax.
     * If the value is below the minimum, it will be clamped to zero.
     * If the value is above the maximum, it will be clamped to the maximum.
     *
     * @param value Value to clamp
     * @param absMin Absolute value of the minimum significant value to return
     * @param absMax Absolute value of the maximum value to return
     * @return The clamped value with the same sign as `value`
     */
    private fun clampMag(value: Float, absMin: Float, absMax: Float): Float {
        val absValue = abs(value)
        if (absValue < absMin) return 0.0f
        return if (absValue > absMax) if (value > 0) absMax else -absMax else value
    }

    private fun distanceInfluenceForSnapDuration(distanceRatio: Float): Float {
        var ratio = distanceRatio
        ratio -= 0.5f // center the values about 0.
        ratio *= (0.3f * Math.PI / 2.0f).toFloat()
        return sin(ratio.toDouble()).toFloat()
    }

    /**
     * Move the captured settling view by the appropriate amount for the current time.
     * If `continueSettling` returns true, the caller should call it again
     * on the next frame to continue.
     *
     * @param deferCallbacks true if state callbacks should be deferred via posted message.
     * Set this to true if you are calling this method from
     * [android.view.View.computeScroll] or similar methods
     * invoked as part of layout or drawing.
     * @return true if settle is still in progress
     */
    fun continueSettling(deferCallbacks: Boolean): Boolean {
        // Make sure, there is a captured view
        if (capturedView == null) {
            return false
        }
        if (viewDragState == STATE_SETTLING) {
            capturedView?.let {
                var keepGoing = mScroller.computeScrollOffset()
                val x = mScroller.currX
                val y = mScroller.currY
                val dx = x - it.left
                val dy = y - it.top
                if (!keepGoing && dy != 0) { //fix #525
                    //Invalid drag state
                    it.top = 0
                    return true
                }
                if (dx != 0) {
                    it.offsetLeftAndRight(dx)
                }
                if (dy != 0) {
                    it.offsetTopAndBottom(dy)
                }
                if (dx != 0 || dy != 0) {
                    mCallback.onViewPositionChanged(it, x, y, dx, dy)
                }
                if (keepGoing && (x == mScroller.finalX) && (y == mScroller.finalY)) {
                    // Close enough. The interpolator/scroller might think we're still moving
                    // but the user sure doesn't.
                    mScroller.abortAnimation()
                    keepGoing = mScroller.isFinished
                }
                if (!keepGoing) {
                    if (deferCallbacks) {
                        mParentView.post(mSetIdleRunnable)
                    } else {
                        setDragState(STATE_IDLE)
                    }
                }
            }
        }
        return viewDragState == STATE_SETTLING
    }

    /**
     * Like all callback events this must happen on the UI thread, but release
     * involves some extra semantics. During a release (mReleaseInProgress)
     * is the only time it is valid to call [.settleCapturedViewAt]
     * or [.flingCapturedView].
     */
    private fun dispatchViewReleased(xVel: Float, yVel: Float) {
        mReleaseInProgress = true
        mCallback.onViewReleased(capturedView, xVel, yVel)
        mReleaseInProgress = false
        if (viewDragState == STATE_DRAGGING) {
            // onViewReleased didn't call a method that would have changed this. Go idle.
            setDragState(STATE_IDLE)
        }
    }

    private fun clearMotionHistory() {
        if (mInitialMotionX == null) {
            return
        }
        mInitialMotionX?.let { Arrays.fill(it, 0f) }
        mInitialMotionY?.let { Arrays.fill(it, 0f) }
        mLastMotionX?.let { Arrays.fill(it, 0f) }
        mLastMotionY?.let { Arrays.fill(it, 0f) }
        Arrays.fill(mInitialEdgesTouched, 0)
        Arrays.fill(mEdgeDragsInProgress, 0)
        Arrays.fill(mEdgeDragsLocked, 0)
        mPointersDown = 0
    }

    private fun ensureMotionHistorySizeForId(pointerId: Int) {
        if (mInitialMotionX == null || (mInitialMotionX?.size ?: 0) <= pointerId) {
            val imx = FloatArray(pointerId + 1)
            val imy = FloatArray(pointerId + 1)
            val lmx = FloatArray(pointerId + 1)
            val lmy = FloatArray(pointerId + 1)
            val iit = IntArray(pointerId + 1)
            val edip = IntArray(pointerId + 1)
            val edl = IntArray(pointerId + 1)
            if (mInitialMotionX != null) {
                mInitialMotionX?.let { System.arraycopy(it, 0, imx, 0, it.size) }
                mInitialMotionY?.let { System.arraycopy(it, 0, imy, 0, it.size) }
                mLastMotionX?.let { System.arraycopy(it, 0, lmx, 0, it.size) }
                mLastMotionY?.let { System.arraycopy(it, 0, lmy, 0, it.size) }
                System.arraycopy(mInitialEdgesTouched, 0, iit, 0, mInitialEdgesTouched.size)
                System.arraycopy(mEdgeDragsInProgress, 0, edip, 0, mEdgeDragsInProgress.size)
                System.arraycopy(mEdgeDragsLocked, 0, edl, 0, mEdgeDragsLocked.size)
            }
            mInitialMotionX = imx
            mInitialMotionY = imy
            mLastMotionX = lmx
            mLastMotionY = lmy
            mInitialEdgesTouched = iit
            mEdgeDragsInProgress = edip
            mEdgeDragsLocked = edl
        }
    }

    private fun saveInitialMotion(x: Float, y: Float, pointerId: Int) {
        ensureMotionHistorySizeForId(pointerId)
        mLastMotionX!![pointerId] = x
        mInitialMotionX!![pointerId] = mLastMotionX!![pointerId]
        mLastMotionY!![pointerId] = y
        mInitialMotionY!![pointerId] = mLastMotionY!![pointerId]
        mInitialEdgesTouched[pointerId] = getEdgesTouched(x.toInt(), y.toInt())
        mPointersDown = mPointersDown or (1 shl pointerId)
    }

    private fun saveLastMotion(ev: MotionEvent) {
        val pointerCount = ev.pointerCount
        for (i in 0 until pointerCount) {
            val pointerId = ev.getPointerId(i)
            val x = ev.getX(i)
            val y = ev.getY(i)
            // Sometimes we can try and save last motion for a pointer never recorded in initial motion. In this case we just discard it.
            if ((mLastMotionX != null) && (mLastMotionY != null
                        ) && (mLastMotionX!!.size > pointerId) && (mLastMotionY!!.size > pointerId)
            ) {
                mLastMotionX!![pointerId] = x
                mLastMotionY!![pointerId] = y
            }
        }
    }

    fun setDragState(state: Int) {
        if (viewDragState != state) {
            viewDragState = state
            mCallback.onViewDragStateChanged(state)
            if (viewDragState == STATE_IDLE) {
                capturedView = null
            }
        }
    }

    /**
     * Attempt to capture the view with the given pointer ID. The callback will be involved.
     * This will put us into the "dragging" state. If we've already captured this view with
     * this pointer this method will immediately return true without consulting the callback.
     *
     * @param toCapture View to capture
     * @param pointerId Pointer to capture with
     * @return true if capture was successful
     */
    private fun tryCaptureViewForDrag(toCapture: View?, pointerId: Int): Boolean {
        if (toCapture === capturedView && activePointerId == pointerId) {
            // Already done!
            return true
        }
        if (toCapture != null && mCallback.tryCaptureView(toCapture, pointerId)) {
            activePointerId = pointerId
            captureChildView(toCapture, pointerId)
            return true
        }
        return false
    }

    /**
     * Check if this event as provided to the parent view's onInterceptTouchEvent should
     * cause the parent to intercept the touch event stream.
     *
     * @param ev MotionEvent provided to onInterceptTouchEvent
     * @return true if the parent view should return true from onInterceptTouchEvent
     */
    fun shouldInterceptTouchEvent(ev: MotionEvent): Boolean {
        val action = ev.actionMasked
        if (action == MotionEvent.ACTION_DOWN) {
            // Reset things for a new event stream, just in case we didn't get
            // the whole previous stream.
            cancel()
        }
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain()
        }
        mVelocityTracker?.addMovement(ev)
        when (action) {
            MotionEvent.ACTION_DOWN -> {
                val x = ev.x
                val y = ev.y
                val pointerId = ev.getPointerId(0)
                saveInitialMotion(x, y, pointerId)
                val toCapture = findTopChildUnder(x.toInt(), y.toInt())

                // Catch a settling view if possible.
                if (toCapture === capturedView && viewDragState == STATE_SETTLING) {
                    tryCaptureViewForDrag(toCapture, pointerId)
                }
            }

            MotionEvent.ACTION_MOVE -> {

                // First to cross a touch slop over a draggable view wins. Also report edge drags.
                val pointerCount = ev.pointerCount
                var i = 0
                while ((i < pointerCount) && (mInitialMotionX != null) && (mInitialMotionY != null)) {
                    val pointerId = ev.getPointerId(i)
                    if (pointerId >= mInitialMotionX!!.size || pointerId >= mInitialMotionY!!.size) {
                        i++
                        continue
                    }
                    val x = ev.getX(i)
                    val y = ev.getY(i)
                    val dx = x - mInitialMotionX!![pointerId]
                    val dy = y - mInitialMotionY!![pointerId]
                    reportNewEdgeDrags(dx, dy, pointerId)
                    if (viewDragState == STATE_DRAGGING) {
                        // Callback might have started an edge drag
                        break
                    }
                    val toCapture = findTopChildUnder(
                        mInitialMotionX!![pointerId].toInt(),
                        mInitialMotionY!![pointerId].toInt()
                    )
                    if (((toCapture != null) && checkTouchSlop(toCapture, dy) &&
                                tryCaptureViewForDrag(toCapture, pointerId))
                    ) {
                        break
                    }
                    i++
                }
                saveLastMotion(ev)
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                cancel()
            }
        }
        return viewDragState == STATE_DRAGGING
    }

    /**
     * Process a touch event received by the parent view. This method will dispatch callback events
     * as needed before returning. The parent view's onTouchEvent implementation should call this.
     *
     * @param ev The touch event received by the parent view
     */
    fun processTouchEvent(ev: MotionEvent) {
        val action = ev.actionMasked
        if (action == MotionEvent.ACTION_DOWN) {
            // Reset things for a new event stream, just in case we didn't get
            // the whole previous stream.
            cancel()
        }
        if (mVelocityTracker == null) {
            mVelocityTracker = VelocityTracker.obtain()
        }
        mVelocityTracker?.addMovement(ev)
        when (action) {
            MotionEvent.ACTION_DOWN -> {
                val x = ev.x
                val y = ev.y
                val pointerId = ev.getPointerId(0)
                val toCapture = findTopChildUnder(x.toInt(), y.toInt())
                saveInitialMotion(x, y, pointerId)

                // Since the parent is already directly processing this touch event,
                // there is no reason to delay for a slop before dragging.
                // Start immediately if possible.
                tryCaptureViewForDrag(toCapture, pointerId)
                mInitialEdgesTouched[pointerId]
            }

            MotionEvent.ACTION_MOVE -> {
                if (viewDragState == STATE_DRAGGING) {
                    val index = ev.findPointerIndex(
                        activePointerId
                    )
                    val x = ev.getX(index)
                    val y = ev.getY(index)
                    val idx = (x - mLastMotionX!![activePointerId]).toInt()
                    val idy = (y - mLastMotionY!![activePointerId]).toInt()
                    capturedView?.let {
                        dragTo(it.left + idx, it.top + idy, idx, idy)
                    }
                    saveLastMotion(ev)
                } else {
                    // Check to see if any pointer is now over a draggable view.
                    val pointerCount = ev.pointerCount
                    var i = 0
                    while (i < pointerCount) {
                        val pointerId = ev.getPointerId(i)
                        val x = ev.getX(i)
                        val y = ev.getY(i)
                        val dx = x - mInitialMotionX!![pointerId]
                        val dy = y - mInitialMotionY!![pointerId]
                        reportNewEdgeDrags(dx, dy, pointerId)
                        if (viewDragState == STATE_DRAGGING) {
                            // Callback might have started an edge drag.
                            break
                        }
                        val toCapture = findTopChildUnder(
                            mInitialMotionX!![pointerId].toInt(),
                            mInitialMotionY!![pointerId].toInt()
                        )
                        if (checkTouchSlop(toCapture, dy) &&
                            tryCaptureViewForDrag(toCapture, pointerId)
                        ) {
                            break
                        }
                        i++
                    }
                    saveLastMotion(ev)
                }
            }

            MotionEvent.ACTION_UP -> {
                if (viewDragState == STATE_DRAGGING) {
                    releaseViewForPointerUp()
                }
                cancel()
            }

            MotionEvent.ACTION_CANCEL -> {
                if (viewDragState == STATE_DRAGGING) {
                    dispatchViewReleased(0f, 0f)
                }
                cancel()
            }
        }
    }

    private fun reportNewEdgeDrags(dx: Float, dy: Float, pointerId: Int) {
        var dragsStarted = 0
        if (checkNewEdgeDrag(dx, dy, pointerId, EDGE_LEFT)) {
            dragsStarted = dragsStarted or EDGE_LEFT
        }
        if (checkNewEdgeDrag(dy, dx, pointerId, EDGE_TOP)) {
            dragsStarted = dragsStarted or EDGE_TOP
        }
        if (checkNewEdgeDrag(dx, dy, pointerId, EDGE_RIGHT)) {
            dragsStarted = dragsStarted or EDGE_RIGHT
        }
        if (checkNewEdgeDrag(dy, dx, pointerId, EDGE_BOTTOM)) {
            dragsStarted = dragsStarted or EDGE_BOTTOM
        }
        if (dragsStarted != 0) {
            mEdgeDragsInProgress[pointerId] = mEdgeDragsInProgress[pointerId] or dragsStarted
        }
    }

    private fun checkNewEdgeDrag(delta: Float, odelta: Float, pointerId: Int, edge: Int): Boolean {
        val absDelta = abs(delta)
        val absODelta = abs(odelta)
        if ((((mInitialEdgesTouched[pointerId] and edge) != edge) || ((mTrackingEdges and edge) == 0) || (
                    (mEdgeDragsLocked[pointerId] and edge) == edge) || (
                    (mEdgeDragsInProgress[pointerId] and edge) == edge) ||
                    (absDelta <= touchSlop && absODelta <= touchSlop))
        ) {
            return false
        }
        return (mEdgeDragsInProgress[pointerId] and edge) == 0 && absDelta > touchSlop
    }

    /**
     * Check if we've crossed a reasonable touch slop for the given child view.
     * If the child cannot be dragged along the horizontal or vertical axis, motion
     * along that axis will not count toward the slop check.
     *
     * @param child Child to check
     * @param dy Motion since initial position along Y axis
     * @return true if the touch slop has been crossed
     */
    private fun checkTouchSlop(child: View?, dy: Float): Boolean {
        if (child == null) {
            return false
        }
        val checkVertical = mCallback.getViewVerticalDragRange(child) > 0
        if (checkVertical) {
            return abs(dy) > touchSlop
        }
        return false
    }

    val isDragging: Boolean
        get() = viewDragState == STATE_DRAGGING

    @Suppress("DEPRECATION")
    private fun releaseViewForPointerUp() {
        mVelocityTracker?.computeCurrentVelocity(1000, mMaxVelocity)
        val xVel = clampMag(
            VelocityTrackerCompat.getXVelocity(mVelocityTracker, activePointerId),
            minVelocity, mMaxVelocity
        )
        val yVel = clampMag(
            VelocityTrackerCompat.getYVelocity(mVelocityTracker, activePointerId),
            minVelocity, mMaxVelocity
        )
        dispatchViewReleased(xVel, yVel)
    }

    private fun dragTo(left: Int, top: Int, dx: Int, dy: Int) {
        capturedView?.let {
            var clampedY = top
            val oldLeft = it.left
            val oldTop = it.top
            if (dx != 0) {
                val clampedX = 0
                it.offsetLeftAndRight(clampedX - oldLeft)
            }
            if (dy != 0) {
                clampedY = mCallback.clampViewPositionVertical(it, top, dy)
                it.offsetTopAndBottom(clampedY - oldTop)
            }
            if (dx != 0 || dy != 0) {
                val clampedDx = left - oldLeft
                val clampedDy = clampedY - oldTop
                mCallback.onViewPositionChanged(
                    it, left, clampedY,
                    clampedDx, clampedDy
                )
            }
        }
    }

    /**
     * Find the topmost child under the given point within the parent view's coordinate system.
     * The child order is determined using [Callback.getOrderedChildIndex].
     *
     * @param x X position to test in the parent's coordinate system
     * @param y Y position to test in the parent's coordinate system
     * @return The topmost child view under (x, y) or null if none found.
     */
    private fun findTopChildUnder(x: Int, y: Int): View? {
        val childCount = mParentView.childCount
        for (i in childCount - 1 downTo 0) {
            val child = mParentView.getChildAt(mCallback.getOrderedChildIndex(i))
            if ((x >= child.left) && (x < child.right) && (
                        y >= child.top) && (y < child.bottom)
            ) {
                return child
            }
        }
        return null
    }

    private fun getEdgesTouched(x: Int, y: Int): Int {
        var result = 0
        if (x < mParentView.left + edgeSize) result = result or EDGE_LEFT
        if (y < mParentView.top + edgeSize) result = result or EDGE_TOP
        if (x > mParentView.right - edgeSize) result = result or EDGE_RIGHT
        if (y > mParentView.bottom - edgeSize) result = result or EDGE_BOTTOM
        return result
    }

    companion object {
        /**
         * A null/invalid pointer ID.
         */
        const val INVALID_POINTER = -1

        /**
         * A view is not currently being dragged or animating as a result of a fling/snap.
         */
        const val STATE_IDLE = 0

        /**
         * A view is currently being dragged. The position is currently changing as a result
         * of user input or simulated user input.
         */
        const val STATE_DRAGGING = 1

        /**
         * A view is currently settling into place as a result of a fling or
         * predefined non-interactive motion.
         */
        const val STATE_SETTLING = 2

        /**
         * Edge flag indicating that the left edge should be affected.
         */
        const val EDGE_LEFT = 1 shl 0

        /**
         * Edge flag indicating that the right edge should be affected.
         */
        const val EDGE_RIGHT = 1 shl 1

        /**
         * Edge flag indicating that the top edge should be affected.
         */
        const val EDGE_TOP = 1 shl 2

        /**
         * Edge flag indicating that the bottom edge should be affected.
         */
        const val EDGE_BOTTOM = 1 shl 3

        private const val EDGE_SIZE = 20 // dp
        private const val BASE_SETTLE_DURATION = 256 // ms
        private const val MAX_SETTLE_DURATION = 600 // ms

        /**
         * Interpolator defining the animation curve for mScroller
         */
        private val sInterpolator: Interpolator =
            Interpolator { time ->
                var t = time
                t -= 1.0f
                t * t * t * t * t + 1.0f
            }

        /**
         * Factory method to create a new ViewDragHelper with the specified interpolator.
         *
         * @param forParent Parent view to monitor
         * @param interpolator interpolator for scroller
         * @param cb Callback to provide information and receive events
         * @return a new ViewDragHelper instance
         */
        private fun create(
            forParent: ViewGroup,
            interpolator: Interpolator?,
            cb: Callback?
        ): ViewDragHelper {
            return ViewDragHelper(forParent.context, forParent, interpolator, cb)
        }

        /**
         * Factory method to create a new ViewDragHelper with the specified interpolator.
         *
         * @param forParent Parent view to monitor
         * @param sensitivity Multiplier for how sensitive the helper should be about detecting
         * the start of a drag. Larger values are more sensitive. 1.0f is normal.
         * @param interpolator interpolator for scroller
         * @param cb Callback to provide information and receive events
         * @return a new ViewDragHelper instance
         */
        fun create(
            forParent: ViewGroup,
            sensitivity: Float,
            interpolator: Interpolator?,
            cb: Callback?
        ): ViewDragHelper {
            val helper = create(forParent, interpolator, cb)
            helper.touchSlop = (helper.touchSlop * (1 / sensitivity)).toInt()
            return helper
        }
    }
}
