function testError() {
	const code = document.getElementById('errorCode').value;

	if (!code) {
		alert('Please select an error code');
		return;
	}

	const resultDiv = document.getElementById('result');
	resultDiv.innerHTML = '<p>Testing error ' + code + '...</p>';

	// Trigger error based on code
	switch(code) {
		case '400':
			// Bad Request - difficult to trigger, show page directly
			window.location.href = '/error_pages/400.html';
			break;
		case '403':
			// Forbidden - difficult to trigger, show page directly
			window.location.href = '/error_pages/403.html';
			break;
		case '404':
			// Request non existante page
			window.location.href = '/nonexistent-page-' + Date.now();
			break;
		case '405':
			// Method not allowed (try PUT on /)
			fetch('/', { method: 'PUT' })
				.then(response => {
					if (response.status === 405) {
						window.location.href = '/error_pages/405.html';
					}
				});
			break;
    	case '504':
			// Test CGI timeout
			resultDiv.innerHTML = '<p>Triggering CGI timeout (this will take 5+ seconds)...</p>';
			setTimeout(() => {
				window.location.href = '/cgi-bin/py/timeout.py';
			}, 500);
		break;
        default:
            // Direct access to error page
            resultDiv.innerHTML = '<p>Redirecting to error page...</p>';
            setTimeout(() => {
                window.location.href = '/error_pages/' + code + '.html';
            }, 500);
	}
}

// Attach event listener when DOM is ready
document.addEventListener('DOMContentLoaded', function() {
    const testBtn = document.getElementById('testBtn');
    if (testBtn) {
        testBtn.addEventListener('click', testError);
    }
});
