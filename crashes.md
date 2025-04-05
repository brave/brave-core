## Crashes

Don't land until these are all resolved!

- [x] New first conversation lets you send page contents even when nothing is attached

    Theory is that this is because the associated_content_manager exists but is empty.

- [x] Setting the associated content to nothing crashes when the conversation is started.

    Probably same root cause as above?

- [x] Clearing data with a multi content conversation open

    This is being caused by shutting down with a reference to a AssociatedContentDelegate still
    live in the AssociatedContentManager. Fix is probably an OnDestroying notification.

- [x] Switching back and forward between a multi conversation and a normal causes a crash

    Looks like metadata size is not being loaded? Maybe it's because we move it somewhere instead of just passing it through?

    Alternatively we could be accidentally updating the metadata to be empty via an |OnAssociatedContentUpdated|?
