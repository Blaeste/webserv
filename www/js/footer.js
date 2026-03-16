function loadFooter() {
	const text = [
		"This project has been created as part of the 42 curriculum by ESchwart GDosh and LMarck"
	];
	const externalLink = [
		{ label: "42 Mulhouse", href: "https://www.42mulhouse.fr/" }
	];

	const footer = `
    <footer>
	<div class="footer-inner">
        <div class="footer-random">${text}</div>
        <div class="footer-links">
          ${externalLink.map(link => `<a href="${link.href}" target="_blank"><img src="/image/42.svg" alt="${link.label}" class="footer-logo"></a>`).join('')}
        </div>
      </div>
    </footer>
  `;


	document.body.insertAdjacentHTML('beforeend', footer);
}

loadFooter();
