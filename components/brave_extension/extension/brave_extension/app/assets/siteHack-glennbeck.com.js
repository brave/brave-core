// Site checks indexOf(' gb3...') - Note the leading space.
// Thus we add it after __a=1
document.cookie = '__a=1'
document.cookie = 'gb3lightbox=1'
document.cookie = 'lightbox_ad=true'
console.log('we set some cookies!!')
