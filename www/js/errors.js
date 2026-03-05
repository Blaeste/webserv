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
			method = 'PUT';
			path = '/';
			description = 'Attempt to use PUT method on the root path (which only allows GET), triggering a 405 Method Not Allowed error';
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
		case '403':
			fetch('/..%2Fconfig/default.conf', { headers: { 'X-Internal-Request': 'true' } })
				.then(response => {
					if (response.status === 403) {
						window.location.href = '/error_pages/403.html';
					} else {
						resultDiv.innerHTML = '<p>Unexpected status: ' + response.status + ' ' + response.statusText + '</p>';
					}
				})
				.catch(err => {
					resultDiv.innerHTML = '<p class="error">Request failed: ' + err.message + '</p>';
				});
			break;
		case '404':
			window.location.href = '/nonexistent-page-' + Date.now();
			break;
		case '405':
			fetch('/', { method: 'DELETE' })
				.then(response => {
					if (response.status === 405) {
						window.location.href = '/error_pages/405.html';
					}
				});
			break;
		case '413': {
			const oversized = new Blob(['a'.repeat(11 * 1024 * 1024)], { type: 'application/octet-stream' });
			resultDiv.innerHTML = '<p>Submitting oversized payload (~11MB) to /uploads to trigger a server-side 413...</p>';
			// If the server closes the connection early for 413, fetch may reject; in both cases we want to show the 413 page.
			fetch('/uploads', { method: 'POST', body: oversized })
				.then(response => {
					if (response.status === 413) {
						window.location.href = '/error_pages/413.html';
					} else {
						resultDiv.innerHTML = '<p>Unexpected status: ' + response.status + ' ' + response.statusText + ' (expected 413). Redirecting to error page anyway...</p>';
						setTimeout(() => {
							window.location.href = '/error_pages/413.html';
						}, 300);
					}
				})
				.catch(err => {
					resultDiv.innerHTML = '<p class="error">Request failed: ' + err.message + ' (treating as 413)</p>';
					setTimeout(() => {
						window.location.href = '/error_pages/413.html';
					}, 200);
				});
			break;
		}
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
