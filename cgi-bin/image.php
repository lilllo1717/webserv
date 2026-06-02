<?php
$body = file_get_contents("php://input");

$uploads_dir = dirname(__FILE__) . "/../uploads/";
$result = file_put_contents($uploads_dir . "cgi_upload.jpg", $body);

header("Content-Type: text/plain");
if ($result === false)
    echo "WRITE FAILED\n";
else
    echo "Wrote: " . $result . " bytes, received: " . strlen($body) . "\n";
?>