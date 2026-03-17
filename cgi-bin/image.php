<?php
header('Content-Type: image/png');
$im = imagecreate(100, 100);
$red = imagecolorallocate($im, 255, 0, 0);
imagefill($im, 0, 0, $red);
imagepng($im);
imagedestroy($im);
?>
