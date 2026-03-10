
class Configs {
  constructor() {
    // const proto = Object.getPrototypeOf(this)
    // Object.getOwnPropertyNames(proto)
    //   .filter(k => k !== 'constructor')
    //   .forEach(k => { this[k] = this[k].bind(this) })
  }

  android() { return this.ios }
  ios() {
    return { configs: [this.android] }
  }
  static() {
    return { configs: [this.android] }
  }
  class() {
    return { configs: [this.android] }
  }
  let() {
    return { configs: [this.android] }
  }
  const() {
    return { configs: [this.android] }
  }
}

c = new Configs()
console.log(c.ios())
console.log(c.android())
console.log(c.static())

console.log(c.ios().configs[0] === c.android)
console.log(c.ios().configs[0] === c.ios().configs[0])
