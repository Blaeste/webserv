#!/usr/bin/env php-cgi
<?php
// Get a unique ID from query string
$query = isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '';
$cgi_id = 'unknown';
if (strpos($query, '=') !== false) {
    $parts = explode('=', $query, 2);
    $cgi_id = $parts[1];
}

// Send headers immediately
header('Content-type: text/html');

// Flush output before sleeping
ob_implicit_flush(true);
ob_end_flush();

// Sleep 1.5 seconds to simulate medium processing
usleep(1500000);

// Send response
echo "<html><body>\n";
echo "<h1>CGI #$cgi_id completed!</h1>\n";
echo "<p>This CGI took 1.5 seconds to process.</p>\n";
echo "</body></html>\n";
?>
