export default function getRandomId() {
  Math.random().toString(36).substr(2, 9)
}