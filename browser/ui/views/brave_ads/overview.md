# Notification Ads Overview

This document is an overview of notification ad popup concepts, terminology, and architecture. The target audience is engineers using or working on notification ads.

For more details about the Chromium UI Platform, see [overview.md].

## Notification Ad Popup

**NotificationAdPopup** is responsible for the display of notification ads and informing **NotificationAdDelegate** listeners of **NotificationAdPopup** events.

**NotificationAdPopup** needs an underlying canvas to paint onto. This is provided by adding **NotificationAdView** to a **views::Widget**.

## Notification Ad View Factory

**NotificationAdViewFactory** is responsible for creating different types of notification ads. i.e., see [text_notification_ad_view.h](./text_notification_ad_view.h).

## Text Notification Ad View

**TextNotificationAdView** is a subclass of **NotificationAdView** that lays out and renders a text notification ad with a solid background, control buttons, title and body text

## Notification Ad View

**NotificationAdView** is a base class for layout, rendering and mouse events of notification ads.

Different **NotificationAdView** subclasses are responsible for implementing their own design. i.e, see [text_notification_ad_view.h](./text_notification_ad_view.h).

## Notification Ad Background Painter

**NotificationAdBackgroundPainter** is responsible for drawing a solid background below any other part of the **NotificationAdView** with rounded corners on macOS and Linux.

## Notification Ad Header View

**NotificationAdHeaderView** is responsible for the layout and rendering of the header which includes the title text.

## Notification Ad Control Buttons View

**NotificationAdControlButtonsView** is responsible for the layout and rendering of the control buttons comprising of a BAT logo and close button.

## Overall structure looks like this

<pre>
┌──────────────────────────────────────────────────────────────────┐
│NotificationAdPopup                                               │
│ ┌──────────────────────────────────────────────────────────────┐ │
│ │NotificationAdView                                            │ │
│ │ ┌──────────────────────────────────────────────────────────┐ │ │
│ │ │NotificationAdBackgroundPainter                           │ │ │
│ │ │ ┌────────────────────────┐┌────────────────────────────┐ │ │ │
│ │ │ │NotificationAdHeaderView││NotificationAdControlButtons│ │ │ │
│ │ │ └────────────────────────┘└────────────────────────────┘ │ │ │
│ │ │ ┌──────────────────────────────────────────────────────┐ │ │ │
│ │ │ │                                                      │ │ │ │
│ │ │ │                                                      │ │ │ │
│ │ │ │                                                      │ │ │ │
│ │ │ └──────────────────────────────────────────────────────┘ │ │ │
│ │ └──────────────────────────────────────────────────────────┘ │ │
│ └──────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
</pre>

[overview.md]: https://chromium.googlesource.com/chromium/src/+/master/docs/ui/views/overview.md
