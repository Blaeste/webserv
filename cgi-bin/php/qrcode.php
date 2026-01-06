#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html; charset=utf-8");

// Get text from query string
$text = isset($_GET['text']) ? $_GET['text'] : 'Hello Webserv!';

// Limit input to 500 char
if (strlen($text) > 500) {
	$text = substr($text, 0, 500);
	$warning = "⚠️ Text truncated to 500 characters";
}

$text = htmlspecialchars($text, ENT_QUOTES, 'UTF-8');

// QR Code API
$qrUrl = 'https://api.qrserver.com/v1/create-qr-code/?size=300x300&data=' . urlencode($text);
?>
<!DOCTYPE html>
<html>
	<head>
		<title>QR Code Generator</title>
		<link rel="stylesheet" href="/css/style.css">
		<link rel="stylesheet" href="/css/qrcode.css">
		<script src="/js/header.js" defer></script>
	</head>

	<body>
		<div class="container">
			<div id="qrGenerator">
				<h1>📱 QR Code Generator</h1>
				<p>Enter text to generate a QR code (500 char max)</p>

				<form method="GET" action="/cgi-bin/php/qrcode.php">
					<input type="text" name="text" value="<?php echo $text; ?>" placeholder="Enter text here..." required>
					<button type="submit">Generate QR Code</button>
				</form>

				<?php if (!empty($text)): ?>
				<div class="qr-result">
					<h2>Your QR Code:</h2>
					<img src="<?php echo $qrUrl; ?>" alt="QR Code" class="qr-image">
					<?php if (isset($warning)): ?>
						<p style="color: orange; font-weight: bold;"><?php echo $warning; ?></p>
					<?php endif; ?>
					<p class="qr-info">
						<strong>Content:</strong> <?php echo $text; ?>
					</p>
					<p class="qr-note">
						<em>Scan this QR code with your phone!</em>
					</p>
				</div>
				<?php endif; ?>
			</div>
		</div>
	</body>
</html>
