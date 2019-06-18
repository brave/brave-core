let n = 0
function append() {
  var node = document.createElement('div')
  var textnode = document.createTextNode(n)
  node.appendChild(textnode)
  document.getElementsByTagName('body')[0].appendChild(node)
  n++
}
function append10() {
  let fragment = document.createDocumentFragment()
  for (let i = 0; i< 10; i++) {
    let p = document.createElement('div')
    p.textContent = n
    fragment.appendChild(p)
    n++
  }
  document.getElementsByTagName('body')[0].appendChild(fragment)
}
