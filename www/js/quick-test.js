function updateRequestPreview() {
	const code = document.getElementById('errorCode').value;
	const detailsDiv = document.getElementById('requestDetails');

	if (!code) {
		detailsDiv.innerHTML = '<p class="placeholder">Select an error code to see the request details</p>';
		return;
	}

	let method = 'GET';
	let path = '';
	let description = '';

	switch (code) {
		case '403':
			path = '/..%2Fconfig/default.conf';
			description = 'Attempt encoded path traversal (../) which the server blocks with a 403';
			break;
		case '404':
			path = '/nonexistent-page-';
			description = 'Request to a non-existent URL to trigger a genuine 404 Not Found error';
			break;
		case '405':
			method = 'DELETE';
			path = '/';
			description = 'Attempt to use DELETE method on the root path (which only allows GET), triggering a 405 Method Not Allowed error';
			break;
		case '413':
			method = 'POST';
			path = '/uploads';
			description = 'Submit an oversized POST to /uploads to exceed the 10MB limit and trigger a genuine 413 response from the server';
			break;
		case '504':
			path = '/cgi-bin/py/timeout.py';
			description = 'Request to a CGI script that will timeout (takes longer than 5 seconds), triggering a 504 Gateway Timeout error';
			break;
		case 'quick-home':
			path = '/';
			description = 'Simple GET request to the homepage.';
			break;
		case 'quick-uploads':
			path = '/uploads (11MB body)';
			description = 'GET request to list uploaded files (or directory listing).';
			break;
		case 'quick-404':
			path = '/nonexistent';
			description = 'GET request to a clearly missing path to trigger a 404.';
			break;
		default:
			path = '/error_pages/' + code + '.html';
			description = 'Direct navigation to the ' + code + ' error page';
	}

	detailsDiv.innerHTML = `
		<div class="request-method">Method: ${method}</div>
		<div class="request-path">${path}</div>
		<div class="request-description">${description}</div>
	`;
}

function testError() {
	const code = document.getElementById('errorCode').value;

	if (!code) {
		alert('Please select a quick test');
		return;
	}

	const responseBox = document.getElementById('response');
	const statusDiv = document.getElementById('statusCode');
	const headersDiv = document.getElementById('responseHeaders');
	const bodyDiv = document.getElementById('responseBody');

	if (responseBox) {
		responseBox.classList.add('visible');
	}
	if (statusDiv) {
		statusDiv.className = '';
		statusDiv.innerHTML = '⏳ Sending request...';
	}
	if (headersDiv) headersDiv.innerHTML = '';
	if (bodyDiv) bodyDiv.innerHTML = '';

	let method = 'GET';
	let url = '';
	let options = { method: 'GET', headers: {} };

	switch (code) {
		case '403':
			url = '/..%2Fconfig/default.conf';
			options.headers['X-Internal-Request'] = 'true';
			break;
		case '404':
			url = '/nonexistent-page-' + Date.now();
			break;
		case '405':
			method = 'DELETE';
			url = '/';
			options.method = method;
			break;
		case '413': {
			const oversized = new Blob(['a'.repeat(11 * 1024 * 1024)], { type: 'application/octet-stream' });
			url = '/uploads';
			method = 'POST';
			options.method = method;
			options.body = oversized;
			break;
		}
		case '504':
			url = '/cgi-bin/py/timeout.py';
			break;
		case 'quick-home':
			url = '/';
			break;
		case 'quick-uploads':
			url = '/uploads';
			break;
		default:
			url = '/error_pages/' + code + '.html';
	}

	const startTime = Date.now();
	fetch(url, options)
		.then(response => response.text().then(text => ({ response, text })))
		.then(({ response, text }) => {
			const responseTime = Date.now() - startTime;
			if (statusDiv) {
				const statusClass = 'status-' + Math.floor(response.status / 100) + 'xx';
				statusDiv.className = statusClass;
				statusDiv.innerHTML = `Status: ${response.status} ${response.statusText} ⏱️ ${responseTime}ms`;
			}

			if (headersDiv) {
				let headersHTML = '<strong>Response Headers:</strong><br>';
				response.headers.forEach((value, key) => {
					headersHTML += `${key}: ${value}<br>`;
				});
				headersDiv.innerHTML = headersHTML;
			}

			if (bodyDiv) {
				try {
					const json = JSON.parse(text);
					bodyDiv.innerHTML = '<strong>Response Body (JSON):</strong><br><pre>' + JSON.stringify(json, null, 2) + '</pre>';
				} catch (e) {
					bodyDiv.innerHTML = '<strong>Response Body:</strong><br><pre>' + escapeHtml(text) + '</pre>';
				}
			}
		})
		.catch(err => {
			if (statusDiv) {
				statusDiv.className = 'status-5xx';
				statusDiv.innerHTML = 'Error: ' + err.message;
			}
			if (headersDiv) headersDiv.innerHTML = '';
			if (bodyDiv) bodyDiv.innerHTML = '';
		});
}

function escapeHtml(text) {
	const div = document.createElement('div');
	div.textContent = text;
	return div.innerHTML;
}

// Attach event listeners when DOM is ready
document.addEventListener('DOMContentLoaded', function () {
	const testBtn = document.getElementById('testBtn');
	const errorSelect = document.getElementById('errorCode');

	if (testBtn) {
		testBtn.addEventListener('click', testError);
	}

	if (errorSelect) {
		errorSelect.addEventListener('change', updateRequestPreview);
	}
});
