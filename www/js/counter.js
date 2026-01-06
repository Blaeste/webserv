// Fetch visit count form server endpoint
fetch('/counter-api')
	.then(response => response.json())
	.then(data => {
		document.getElementById('counter').textContent = data.visitCount;
		document.getElementById('sessionId').textContent = data.sessionId.substring(0, 16) + '...';
	})
	.catch(err => {
		document.getElementById('counter').textContent= 'Error';
		console.log('Counter error', err);
	});
