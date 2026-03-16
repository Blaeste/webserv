function loadHeader() {
	ensureFavicon();

	const header = `
		<header>
			<button id="header-menu-toggle" class="header-menu-toggle" aria-label="Open navigation">
				<span class="header-menu-bars"></span>
			</button>
			<a href="/" class="header-banner-link">
				<img src="/image/webservbanner.png" alt="Webserv home" class="header-logo">
			</a>
			<nav class="main-nav">
				<a href="/">Home</a>
				<a href="/files.html">Files</a>
				<a href="/qrcode.html">QR Code</a>
				<a href="/quicktest.html">Quicktest</a>
				<a href="/http-test.html">HTTP</a>
				<a href="/contact.html">Contact</a>
				<a href="/about.html">About</a>
			</nav>
			<div class="header-info">
				<div id="svisit-counter" class="visit-counter"><span id ="visit-count">-</span> pages visited</div>
				<div id="status-pill" class="status-pill status-loading">Loading status...</div>
				<button id="dark-mode-toggle" class="header-toggle-btn" type="button">Dark mode</button>
				<button id="marvin-deactivate" class="header-toggle-btn" type="button">Deactivate Marvin</button>
			</div>
			<div id="header-menu" class="header-menu">
				<a href="/">Home</a>
				<a href="/files.html">Files</a>
				<a href="/qrcode.html">QR Code</a>
				<a href="/quicktest.html">Quicktest</a>
				<a href="/http-test.html">HTTP</a>
				<a href="/contact.html">Contact</a>
				<a href="/about.html">About</a>
			</div>
		</header>
	`;

	document.body.insertAdjacentHTML('afterbegin', header);
	setupHeaderMenu();
	updateStatusCode();
	updateVisitCounter();
	setupPreferenceButtons();
}

function ensureFavicon() {
	if (!document.head.querySelector('link[rel="icon"]')) {
		const link = document.createElement('link');
		link.rel = 'icon';
		link.href = '/favicon.ico';
		link.sizes = 'any';
		document.head.appendChild(link);
	}
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
			let label = 'status code: ' + code + ' ' + (response.statusText || '');
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
			window.__sessionVisitCount = data.visitCount;
			window.__sessionId = data.sessionId;
			applySessionPreferences();
		})
		.catch(() => {
			const countSpan = document.getElementById('visit-count');
			if (countSpan) countSpan.textContent = '?';
		});
}

function setupHeaderMenu() {
	const toggle = document.getElementById('header-menu-toggle');
	const menu = document.getElementById('header-menu');
	if (!toggle || !menu) return;

	toggle.addEventListener('click', function () {
		menu.classList.toggle('open');
	});

	menu.addEventListener('click', function (e) {
		if (e.target.tagName === 'A') {
			menu.classList.remove('open');
		}
	});
}

// ---------------------------------------------------------------------------
// Preferences: cookies, dark mode, Marvin
// ---------------------------------------------------------------------------

function setCookie(name, value, days) {
	let expires = '';
	if (typeof days === 'number') {
		const date = new Date();
		date.setTime(date.getTime() + days * 24 * 60 * 60 * 1000);
		expires = '; expires=' + date.toUTCString();
	}
	document.cookie = name + '=' + encodeURIComponent(value) + expires + '; Path=/';
}

function getCookie(name) {
	const nameEQ = name + '=';
	const ca = document.cookie.split(';');
	for (let i = 0; i < ca.length; i++) {
		let c = ca[i];
		while (c.charAt(0) === ' ') c = c.substring(1, c.length);
		if (c.indexOf(nameEQ) === 0) return decodeURIComponent(c.substring(nameEQ.length, c.length));
	}
	return null;
}

function eraseCookie(name) {
	setCookie(name, '', -1);
}

function setupPreferenceButtons() {
	const darkBtn = document.getElementById('dark-mode-toggle');
	const marvinBtn = document.getElementById('marvin-deactivate');

	if (darkBtn) {
		const initialDark = getCookie('dark_mode') === '1';
		applyDarkMode(initialDark);
		darkBtn.textContent = initialDark ? 'Light mode' : 'Dark mode';
		darkBtn.addEventListener('click', function () {
			const enabled = document.body.classList.toggle('dark-mode');
			setCookie('dark_mode', enabled ? '1' : '0', 365);
			darkBtn.textContent = enabled ? 'Light mode' : 'Dark mode';
		});
	}

	if (marvinBtn) {
		marvinBtn.addEventListener('click', function () {
			if (marvinBtn.disabled)
				return;

			// Ensure we have session info; if not, refetch quickly
			if (!window.__sessionId || typeof window.__sessionVisitCount !== 'number') {
				fetch('/counter-api', { headers: { 'X-Internal-Request': 'true' } })
					.then(r => r.json())
					.then(data => {
						window.__sessionId = data.sessionId;
						window.__sessionVisitCount = data.visitCount;
						performMarvinDeactivation(marvinBtn);
					});
			} else {
				performMarvinDeactivation(marvinBtn);
			}
		});
	}

	// Apply existing Marvin state (if any)
	applySessionPreferences();
}

function applyDarkMode(enabled) {
	if (enabled)
		document.body.classList.add('dark-mode');
	else
		document.body.classList.remove('dark-mode');
}

function performMarvinDeactivation(button) {
	const sessionId = window.__sessionId;
	const visitCount = window.__sessionVisitCount;
	if (!sessionId || typeof visitCount !== 'number')
		return;

	// If already deactivated for this session, do nothing
	const storedSession = getCookie('marvin_session_id');
	const marvinDisabled = getCookie('marvin_disabled') === '1';
	if (marvinDisabled && storedSession === sessionId) {
		button.disabled = true;
		button.textContent = 'Marvin deactivated';
		return;
	}

	setCookie('marvin_disabled', '1', 365);
	setCookie('marvin_session_id', sessionId, 365);
	setCookie('marvin_origin_path', window.location.pathname, 365);
	setCookie('marvin_deactivate_visit', String(visitCount), 365);
	setCookie('marvin_family_shown', '0', 365);

	button.disabled = true;
	button.textContent = 'Marvin deactivated';
	applySessionPreferences();
}

function clearMarvinCookies() {
	eraseCookie('marvin_disabled');
	eraseCookie('marvin_session_id');
	eraseCookie('marvin_origin_path');
	eraseCookie('marvin_deactivate_visit');
	eraseCookie('marvin_family_shown');
}

function applySessionPreferences() {
	const sessionId = window.__sessionId;
	const visitCount = window.__sessionVisitCount;
	if (!sessionId || typeof visitCount !== 'number')
		return;

	const storedSession = getCookie('marvin_session_id');
	const marvinDisabled = getCookie('marvin_disabled') === '1';
	const originPath = getCookie('marvin_origin_path');
	const deactivateVisitStr = getCookie('marvin_deactivate_visit');
	const familyShownStr = getCookie('marvin_family_shown');

	const marvinBtn = document.getElementById('marvin-deactivate');
	if (marvinBtn) {
		if (marvinDisabled && storedSession === sessionId) {
			marvinBtn.disabled = true;
			marvinBtn.textContent = 'Marvin deactivated';
		} else {
			marvinBtn.disabled = false;
			marvinBtn.textContent = 'Deactivate Marvin';
		}
	}

	if (!marvinDisabled || storedSession !== sessionId || !deactivateVisitStr) {
		// Either not deactivated for this session or stale cookies from old session
		if (storedSession && storedSession !== sessionId)
			clearMarvinCookies();
		resetMarvinDom();
		return;
	}

	const deactivateVisit = parseInt(deactivateVisitStr, 10);
	if (isNaN(deactivateVisit))
		return;

	let familyShown = familyShownStr === '1';
	const delta = visitCount - deactivateVisit;
	const marvinBlocks = document.querySelectorAll('.marvin');
	const isOriginPage = originPath && window.location.pathname === originPath;

	// 1) Easter egg: if 14th or 15th visit after deactivation is on the
	// origin page, show marvin_easter_egg, then reset Marvin state.
	if (isOriginPage && (delta === 4 || delta === 5)) {
		marvinBlocks.forEach(function (block) {
			block.style.display = '';
			let img = block.querySelector('img');
			if (!img) {
				img = document.createElement('img');
				block.innerHTML = '';
				block.appendChild(img);
			}
			img.src = '/image/marvin_easter_egg.png';
			img.alt = 'Marvin Easter Egg';
		});

		// Reset deactivate Marvin mode for this session
		clearMarvinCookies();
		document.documentElement.classList.remove('marvin-disabled');
		if (marvinBtn) {
			marvinBtn.disabled = false;
			marvinBtn.textContent = 'Deactivate Marvin';
		}
		return;
	}

	// 2) Global default while Marvin is deactivated: hide all Marvin blocks
	marvinBlocks.forEach(function (block) {
		block.style.display = 'none';
	});

	// 3) At the 14th visit after deactivation, show familly_marvin once
	// on a non-origin page (if any Marvin block exists).
	if (!isOriginPage && delta === 4 && !familyShown && marvinBlocks.length > 0) {
		const block = marvinBlocks[0];
		block.style.display = '';
		let img = block.querySelector('img');
		if (!img) {
			img = document.createElement('img');
			block.innerHTML = '';
			block.appendChild(img);
		}
		img.src = '/image/familly_marvin.png';
		img.alt = 'Family Marvin';
		setCookie('marvin_family_shown', '1', 365);
		return;
	}

	// 4) On the origin page, show marvin_dead for the first 15 visits
	// after deactivation (unless Easter egg already handled above).
	if (isOriginPage && delta >= 0 && delta < 15) {
		marvinBlocks.forEach(function (block) {
			block.style.display = '';
			let img = block.querySelector('img');
			if (!img) {
				img = document.createElement('img');
				block.innerHTML = '';
				block.appendChild(img);
			}
			img.src = '/image/marvin_dead.png';
			img.alt = 'Dead Marvin';
		});
	}
}

function resetMarvinDom() {
	// Reset only display property; original images are defined in HTML
	const marvinBlocks = document.querySelectorAll('.marvin');
	marvinBlocks.forEach(function (block) {
		block.style.display = '';
	});
}

loadHeader();
