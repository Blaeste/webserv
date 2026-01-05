function loadHeader() {
	const header = `
		<header>
			<nav>
				<a href="/">Home</a>
				<a href="/upload.html">Upload</a>
				<a href="/files.html">Files</a>
				<a href="/about.html">About</a>
			</nav>
		</header>
	`;

	document.body.insertAdjacentHTML('afterbegin', header);
}

loadHeader();
