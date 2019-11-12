// Stop the google mic from showing up.
// https://github.com/brave/brave-browser/issues/2690
document.querySelector('[aria-label="Search by voice"]').style.visibility = 'hidden';
