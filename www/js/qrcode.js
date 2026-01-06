document.addEventListener('DOMContentLoaded', function() {
	const form = document.getElementById('qrForm');
	const textInput = document.getElementById('qrText');
	const resultDiv = document.getElementById('result');

	form.addEventListener('submit', function(e) {
		e.preventDefault();

		const text = textInput.value.trim();

		if (!text) {
			resultDiv.innerHTML = '<p style="color: red;">Please enter some text</p>';
			return;
		}

		// Show loading
		resultDiv.innerHTML = '<p>Generating QR Code...</p>';

		// Call PHP CGI
		fetch('/cgi-bin/php/qrcode.php?text=' + encodeURIComponent(text))
			.then(response => response.json())
			.then(data => {
				// Check for error from PHP
				if (data.error) {
					resultDiv.innerHTML = '<p style="color: red;">' + data.error + '</p>';
					return;
				}

				let html = '<h2>Your QR Code:</h2>';
				html += '<img src="' + data.url + '" alt="QR Code" class="qr-image">';
				html += '<p class="qr-info"><strong>Content:</strong> ' + data.text + '</p>';

				if (data.warning)
					html += '<p style="color: orange; font-weight: bold;">' + data.warning + '</p>';

				html += '<p class="qr-note"><em>Scan this QR code with your phone!</em></p>';
				resultDiv.innerHTML = html;
			})
			.catch(error => {
				resultDiv.innerHTML = '<p style="color: red;">Error generating QR code: ' + error.message + '</p>';
			});
	});
});
