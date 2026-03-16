<?php
echo "Content-Type: text/plain\r\n";
echo "\r\n";
echo "REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "SCRIPT_FILENAME: " . $_SERVER['SCRIPT_FILENAME'] . "\n";
echo "QUERY_STRING: " . $_SERVER['QUERY_STRING'] . "\n";
echo "Hello from CGI!\n";
?>
