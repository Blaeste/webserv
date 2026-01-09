document.addEventListener('DOMContentLoaded', function() {
	const sendBtn = document.getElementById('sendBtn');
	sendBtn.addEventListener('click', sendRequest);
});

async function sendRequest() {
	const method = document.getElementById('method').value;
	const url = document.getElementById('url').value;
	const body = document.getElementById('requestBody').value;

	const responseBox = document.getElementById('response');
	const statusDiv = document.getElementById('statusCode');
	const headersDiv = document.getElementById('responseHeaders');
	const bodyDiv = document.getElementById('responseBody');

	// Security only on uploads
	/*if ((method === 'POST' || method === 'PUT' || method === 'DELETE') && !url.startsWith('/uploads/')) {
		responseBox.classList.add('visible');
		statusDiv.className = 'status-4xx';
		statusDiv.innerHTML = '⚠️ POST, PUT and DELETE is only allowed on /uploads/* paths for safety';
		headersDiv.innerHTML = '';
		bodyDiv.innerHTML = '<p>Please use a path starting with /uploads/</p>';
		return;
	}*/

	// Show loading
	responseBox.classList.add('visible');
	statusDiv.innerHTML = '⏳ Sending request...';
	statusDiv.className = '';

	try {
		const options = {
			method: method,
			headers: {}
		};

		// Add body for POST/PUT
		if ((method == 'POST' || method == 'PUT') && body) {
			options.body = body;
			options.headers['Content-Type'] = 'application/json';
		}

		const startTime = Date.now();
		const response = await fetch(url, options);
		const responseTime = Date.now() - startTime;

		// Display status code
		const statusClass = 'status-' + Math.floor(response.status / 100) + 'xx';
		statusDiv.className = statusClass;
		statusDiv.innerHTML = `Status: ${response.status} ${response.statusText} ⏱️ ${responseTime}ms`;

		// Display headers
		let headersHTML = '<strong>Response Headers:</strong><br>';
		response.headers.forEach((value, key) => {
			headersHTML += `${key}: ${value}<br>`;
		});
		headersDiv.innerHTML = headersHTML;

		// Display body
		const responseText = await response.text();

		try {
			// try to parse JSON for pretty print
			const json = JSON.parse(responseText);
			bodyDiv.innerHTML = '<strong>Response Body (JSON):</strong><br><pre>' + JSON.stringify(json, null, 2) + '</pre>';

		} catch {
			// Error => display a text
			bodyDiv.innerHTML = '<strong>Response Body:</strong><br><pre>' + escapeHtml(responseText) + '</pre>';
		}

	} catch (error) {
		statusDiv.className = 'status-5xx';
		statusDiv.innerHTML = 'Error: ' + error.message;
		headersDiv.innerHTML = '';
		bodyDiv.innerHTML = '';
	}
}

function escapeHtml(text) {
	const div = document.createElement('div');
	div.textContent = text;
	return div.innerHTML;
}

// Quick test function
function quickTest(method, url, body = '') {
	document.getElementById('method').value = method;
	document.getElementById('url').value = url;
	document.getElementById('requestBody').value = body;
	sendRequest();
}
