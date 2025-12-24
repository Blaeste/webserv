/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.php                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/24 19:56:35 by gdosch            #+#    #+#             */
/*   Updated: 2025/12/24 20:46:19 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

<?php
header("Content-Type: text/html");

echo "<html><body>";
echo "<h1>PHP CGI Test</h1>";
echo "<p>Method: " . ($_SERVER['REQUEST_METHOD'] ?? 'N/A') . "</p>";
echo "<p>Query: " . ($_SERVER['QUERY_STRING'] ?? 'N/A') . "</p>";
echo "<p>Script: " . ($_SERVER['SCRIPT_FILENAME'] ?? 'N/A') . "</p>";
echo "</body></html>";
?>
