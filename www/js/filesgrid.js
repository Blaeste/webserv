function loadFilesList() {

	const fileListDiv = document.getElementById('fileList');

	if (!fileListDiv) return; // error if not on the good page

	// Fetch and display list of files
	fetch('/uploads', {
		headers: { 'X-Internal-Request': 'true' }
	})
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
							<button class="delete-btn" data-file="/uploads/${filename}" data-filename="${filename}">
								Delete
    						</button>
						</li>
					`;
				} else {
					fileListHTML += `
						<li class="file-item">
							<a href="${href}" target="_blank">
								<div class="file-icon">📄</div>
								<span>${filename}</span>
							</a>
							<button class="delete-btn" data-file="/uploads/${filename}" data-filename="${filename}">
								Delete
    						</button>
						</li>
					`;
				}
			});
			fileListHTML += '</ul>';

			fileListDiv.innerHTML = fileListHTML;

			const deleteButtons = document.querySelectorAll('.delete-btn');
			deleteButtons.forEach(btn => {
				btn.addEventListener('click', function(e) {
					e.preventDefault();
					const filepath = this.dataset.file;
					const filename = this.dataset.filename;
					deleteFile(filepath, filename);
				});
			});
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

function deleteFile(filepath, filename) {

	// Ask confirmation
	if (!confirm(`Voulez-vous vraiment supprimer "${filename}" ?`)) {
		return;
	}

	// Send DELETE to server
	fetch(filepath, {
		method: 'DELETE'
	})
	.then(response => {
		if (response.ok || response.status === 204) {
			alert(`✓ "${filename}" supprimé avec succès!`);
			loadFilesList();
		} else {
			alert(`✗ Erreur lors de la suppression`);
		}
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
