<?php
header("Content-Type: text/html");

echo "<html><body>";
echo "<h1>PHP CGI Test</h1>";
echo "<p>Method: " . ($_SERVER['REQUEST_METHOD'] ?? 'N/A') . "</p>";
echo "<p>Query: " . ($_SERVER['QUERY_STRING'] ?? 'N/A') . "</p>";
echo "<p>Script: " . ($_SERVER['SCRIPT_FILENAME'] ?? 'N/A') . "</p>";
echo "</body></html>";
?>
