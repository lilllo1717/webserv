<?php
$body = file_get_contents("php://input");

file_put_contents("/home/tignatov/webserv_git/uploads/cgi_upload.jpg", $body);

header("Content-Type: text/plain");
echo "Received bytes: " . strlen($body) . "\n";
?>