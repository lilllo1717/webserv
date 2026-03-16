<?php
header('Content-Type: image/png');
$im = imagecreate(100, 100);
imagecolorallocate($im, 255, 0, 0);  // Red background
header('Content-Length: filesize');
imagepng($im);
imagedestroy($im);
?>
