// Stop the google mic from showing up.
// https://github.com/brave/brave-browser/issues/2690

var selection = document.querySelector('[aria-label="Search by voice"]')!== null;
if (selection) {
  document.querySelector('[aria-label="Search by voice"]').style.visibility = 'hidden';
} else {
  console.warn('The element aria-label does not exist in the page.');
}

