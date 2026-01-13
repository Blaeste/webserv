function loadHeader() {
	const header = `
		<header>
			<a href="/" class="header-banner-link">
				<img src="/image/webservbanner.png" alt="Webserv home" class="header-logo">
			</a>
			<nav>
				<a href="/">Home</a>
				<a href="/files.html">Files</a>
				<a href="/qrcode.html">QR Code</a>
				<a href="/errors.html">Errors</a>
				<a href="/http-test.html">HTTP</a>
				<a href="/contact.html">Contact</a>				
				<a href="/about.html">About</a>
			</nav>
			<div class="header-info">
				<div id="svisit-counter" class="visit-counter"><span id ="visit-count">-</span> pages visited</div>
				<div id="status-pill" class="status-pill status-loading">Loading status...</div>
			</div>
		</header>
	`;

	document.body.insertAdjacentHTML('afterbegin', header);
	updateStatusCode();
	updateVisitCounter();
}

function updateStatusCode() {
	const pill = document.getElementById('status-pill');
	if (!pill) return;

	fetch(window.location.pathname, {
		cache: 'no-store',
		headers: { 'X-Internal-Request': 'true' }
	})
		.then(response => {
			const code = response.status;
			let label ='status code: ' + code + ' ' + (response.statusText || '');
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

function updateVisitCounter() {
	fetch('/counter-api', {
		headers: { 'X-Internal-Request': 'true' }
	})
		.then(response => response.json())
		.then(data => {
			const countSpan = document.getElementById('visit-count');
			if (countSpan) countSpan.textContent = data.visitCount;
		})
		.catch(() => {
			const countSpan = document.getElementById('visit-count');
			if (countSpan) countSpan.textContent = '?';
		});
}

loadHeader();
