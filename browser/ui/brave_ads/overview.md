# Ad Notifications Overview

This document is an overview of ad notification popup concepts, terminology, and architecture. The target audience is engineers using or working on ad notifications.

For more details about the Chromium UI Platform, see [overview.md].

## Ad Notification Popup Views

**AdNotificationPopupView** is responsible for the display of ad notifications and informing **AdNotificationDelegate** listeners of **AdNotificationPopupView** events.

**AdNotificationPopupView** needs an underlying canvas to paint onto. This is provided by adding **AdNotificationView** to a **views::widget**.

## Ad Notification View Factory

**AdNotificationViewFactory** is responsible for creating different types of ad notifications. i.e., see [text_ad_notification_view.h](./text_ad_notification_view.h).

## Text Ad Notification View

**TextAdNotificationView** is a subclass of **AdNotificationView** that lays out and renders a text ad notification with a solid background, control buttons, title and body text

## Ad Notification View

**AdNotificationView** is a base class for layout, rendering and mouse events of ad notifications.

Different **AdNotificationView** subclasses are responsible for implementing their own design. i.e, see [text_ad_notification_view.h](./text_ad_notification_view.h).

## Ad Notification Background Painter

**AdNotificationBackgroundPainter** is responsible for drawing a solid background below any other part of the **AdNotificationView** with rounded corners on macOS and Linux.

## Ad Notification Header View

**AdNotificationHeaderView** is responsible for the layout and rendering of the header which includes the title text.

## Ad Notification Control Buttons View

**AdNotificationControlButtonsView** is responsible for the layout and rendering of the control buttons comprising of a BAT logo and close button.

## Overall structure looks like this

<pre>
┌──────────────────────────────────────────────────────────────────┐
│AdNotificationPopup                                               │
│ ┌──────────────────────────────────────────────────────────────┐ │
│ │AdNotificationView                                            │ │
│ │ ┌──────────────────────────────────────────────────────────┐ │ │
│ │ │AdNotificationBackgroundPainter                           │ │ │
│ │ │ ┌────────────────────────┐┌────────────────────────────┐ │ │ │
│ │ │ │AdNotificationHeaderView││AdNotificationControlButtons│ │ │ │
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
