import getBraveNewsController from ".";
import {
  BraveNewsControllerRemote,
  FeedListenerInterface,
  FeedListenerReceiver
} from "../../../../../out/Component/gen/brave/components/brave_today/common/brave_news.mojom.m";

export const addFeedListener = (listener: (feedHash: string) => void) =>
  new (class implements FeedListenerInterface {
    #receiver = new FeedListenerReceiver(this);
    #controller: BraveNewsControllerRemote;

    constructor() {
      this.#controller = getBraveNewsController();

      if (process.env.NODE_ENV !== "test") {
        this.#controller.addFeedListener(
          this.#receiver.$.bindNewPipeAndPassRemote()
        );
      }
    }

    onUpdateAvailable(feedHash: string): void {
      listener(feedHash);
    }
  })();
