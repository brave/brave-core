import {
  BraveNewsControllerRemote,
  Publisher,
  UserEnabled
} from "gen/brave/components/brave_today/common/brave_news.mojom.m";
import getBraveNewsController from ".";

type ChangeEvent<Entity> = {
  inserted: { [key: string]: Entity },
  updated: { [key: string]: Partial<Entity> },
  deleted: string[]
}

type Listener<Entity> = (e: ChangeEvent<Entity>) => void;
interface ControllerInterface<Entity> {
  addListener(listener: Listener<Entity>): void;
  removeListener(listener: Listener<Entity>): void;
}

function eagerUpdate<T>(
  target: any,
  propertyKey: string,
  descriptor: TypedPropertyDescriptor<T>
) {
}

function eagerDelete<T>(target: any, propertyKey: string, descriptor: TypedPropertyDescriptor<T>) {

}

function eagerInsert<T>(target: any, propertyKey: string, descriptor: TypedPropertyDescriptor<T>) {

}

class CachingWrapper<Entity> {
  #cache: { [key: string]: Entity } = {};
  #getAll: () => Promise<{ [key: string]: Entity }>;

  constructor(getAll: () => Promise<{ [key: string]: Entity }>) {
    this.#getAll = getAll;

    this.getAll();
  }

  get(id: string) {
    return this.#cache[id];
  }

  getAll() {
    this.#getAll().then(c => this.#cache = c);
    return this.#cache;
  }
}

class PublisherCachingWrapper extends CachingWrapper<
  BraveNewsControllerRemote,
  Publisher,
  "publisherId"
> {
  constructor() {
    super(getBraveNewsController(), "publisherId");
  }

  @eagerUpdate
  setPublisherPref(publisherId: string, newStatus: UserEnabled) {
    this.api.setPublisherPref(publisherId, newStatus);
  }
}
