function loadHeader() {
	const header = `
		<header>
			<a href="/" class="header-banner-link">
				<img src="/image/webservbanner.png" alt="Webserv home" class="header-logo">
			</a>
			<nav>
				<a href="/">Home</a>
				<a href="/files.html">Files</a>
				<a href="/about.html">About</a>
				<a href="/qrcode.html">QR Code</a>
				<a href="/counter.html">Visite count</a>
				<a href="/errors.html">Errors</a>
				<a href="/http-test.html">HTTP</a>
				<a href="/contact.html">Contact</a>
			</nav>
			<div id="status-pill" class="status-pill status-loading">Loading status...</div>
		</header>
	`;

	document.body.insertAdjacentHTML('afterbegin', header);
	updateStatusCode();
}

function updateStatusCode() {
	const pill = document.getElementById('status-pill');
	if (!pill) return;

	fetch(window.location.pathname, { cache: 'no-store' })
		.then(response => {
			const code = response.status;
			let label = code + ' ' + (response.statusText || '');
			pill.textContent = label.trim();

			pill.classList.remove('status-loading', 'status-2xx', 'status-4xx', 'status-5xx');
			if (code >= 200 && code < 300) pill.classList.add('status-2xx');
			else if (code >= 400 && code < 500) pill.classList.add('status-4xx');
			else if (code >= 500) pill.classList.add('status-5xx');
		})
		.catch(() => {
			pill.textContent = 'Status unavailable';
			pill.classList.remove('status-loading');
		});
}

loadHeader();
