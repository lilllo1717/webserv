<?php
$body = file_get_contents("php://input");

$result = file_put_contents("/home/jilustre/Projects/webserv_git/uploads/cgi_upload.jpg", $body);

header("Content-Type: text/plain");
if ($result === false)
    echo "WRITE FAILED\n";
else
    echo "Wrote: " . $result . " bytes, received: " . strlen($body) . "\n";
?>