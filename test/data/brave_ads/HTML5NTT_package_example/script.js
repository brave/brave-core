function animate() {
  const box = document.querySelector('.animated-box');

  // Remove wobble animation class after animation ends
  box.addEventListener('animationend', () => {
    box.classList.remove('mouseout');
  });

  // Add mouseover and mouseout events
  box.addEventListener('mouseover', () => {
    box.classList.remove('mouseout'); // Stop wobble
  });

  box.addEventListener('mouseout', () => {
    box.classList.add('mouseout'); // Trigger wobble on mouseout
  });
}
