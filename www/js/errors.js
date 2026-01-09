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
		case '400':
			path = '/error_pages/400.html';
			description = 'Direct navigation to 400 error page (actual bad request errors are difficult to trigger from browser)';
			break;
		case '403':
			path = '/error_pages/403.html';
			description = 'Direct access to the 403 Forbidden error page. Note: Path traversal attacks (../...) are blocked server-side, but browsers/curl normalize URLs before sending. Use netcat for raw testing: echo -e "GET /www/../config/default.conf HTTP/1.1\\r\\nHost: localhost\\r\\n\\r\\n" | nc localhost 8080';
			break;
		case '404':
			path = '/nonexistent-page-' + Date.now();
			description = 'Request to a non-existent URL to trigger a genuine 404 Not Found error';
			break;
		case '405':
			method = 'PUT';
			path = '/';
			description = 'Attempt to use PUT method on the root path (which only allows GET), triggering a 405 Method Not Allowed error';
			break;
		case '504':
			path = '/cgi-bin/py/timeout.py';
			description = 'Request to a CGI script that will timeout (takes longer than 5 seconds), triggering a 504 Gateway Timeout error';
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
		alert('Please select an error code');
		return;
	}

	const resultDiv = document.getElementById('result');
	resultDiv.innerHTML = '<p>Testing error ' + code + '...</p>';

	// Trigger error based on code
	switch (code) {
		case '400':
			window.location.href = '/error_pages/400.html';
			break;
		case '403':
			window.location.href = '/error_pages/403.html';
			break;
		case '404':
			window.location.href = '/nonexistent-page-' + Date.now();
			break;
		case '405':
			fetch('/', { method: 'PUT' })
				.then(response => {
					if (response.status === 405) {
						window.location.href = '/error_pages/405.html';
					}
				});
			break;
		case '504':
			resultDiv.innerHTML = '<p>Triggering CGI timeout (this will take 5+ seconds)...</p>';
			setTimeout(() => {
				window.location.href = '/cgi-bin/py/timeout.py';
			}, 500);
			break;
		default:
			resultDiv.innerHTML = '<p>Redirecting to error page...</p>';
			setTimeout(() => {
				window.location.href = '/error_pages/' + code + '.html';
			}, 500);
	}
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
