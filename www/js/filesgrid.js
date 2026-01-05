function loadFilesList() {

	const fileListDiv = document.getElementById('fileList');

	if (!fileListDiv) return; // error if not on the good page

	// Fetch and display list of files
	fetch('/uploads')
		.then(response => response.text())
		.then(html => {
			const parser = new DOMParser();
			const doc = parser.parseFromString(html, 'text/html');
			const links = doc.querySelectorAll('ul li a');

			let fileListHTML = '<ul class="file-grid">';
			links.forEach(link => {
				const filename = link.textContent;
				const href = link.getAttribute('href');
				const ext = filename.split('.').pop().toLowerCase();

				// Display image or file icone
				if (['jpg', 'jpeg', 'png', 'gif', 'webp'].includes(ext)) {
					fileListHTML += `
						<li class="file-item">
							<a href="${href}" target="_blank">
								<img src="${href}" alt="${filename}">
								<span>${filename}</span>
							</a>
						</li>
					`;
				} else {
					fileListHTML += `
						<li class="file-item">
							<a href="${href}" target="_blank">
								<div class="file-icon">📄</div>
								<span>${filename}</span>
							</a>
						</li>
					`;
				}
			});
			fileListHTML += '</ul>';

			fileListDiv.innerHTML = fileListHTML;
		})
		.catch(err => {
			console.error('Error:', err);
			fileListDiv.innerHTML = '<p>Error loading files</p>';
		});
}

function setupUploadForm() {
	const uploadForm = document.getElementById('uploadForm');

	if (!uploadForm) return; // error management

	uploadForm.addEventListener('submit', function(e) {

		e.preventDefault();

		const formData = new FormData(this);
		const statusDiv = document.getElementById('uploadStatus');

		statusDiv.innerHTML = '<p>Uploading...</p>';

		fetch('/uploads', {
			method: 'POST',
			body: formData
		})
		.then(response => {
			if (response.ok) {
				statusDiv.innerHTML = '<p style="color:green">✓ Upload successful!</p>';

				// reload file
				setTimeout(() => {
					loadFilesList();
					statusDiv.innerHTML = '';
					uploadForm.reset();
				}, 1000);
			} else {
				statusDiv.innerHTML = '<p style="color:red">✗ Upload failed</p>';
			}
		})
		.catch(err => {
			statusDiv.innerHTML = '<p style="color:red">✗ Error: ' + err + '</p>';
		});
	});
}

loadFilesList();
setupUploadForm();

// Display file names

const fileInput = document.getElementById('fileInput');

if (fileInput) {
	fileInput.addEventListener('change', function() {
		const fileNames = Array.from(this.files).map(f => f.name).join(', ');
		document.getElementById('fileNames').textContent = fileNames || 'No files selected';
	});
}
