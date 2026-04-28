<?php
header('Content-Type: text/plain');
echo "REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "QUERY_STRING: " . $_SERVER['QUERY_STRING'] . "\n";
echo "SCRIPT_FILENAME: " . $_SERVER['SCRIPT_FILENAME'] . "\n";
echo "SERVER_NAME: " . $_SERVER['SERVER_NAME'] . "\n";
echo "name param: " . ($_GET['name'] ?? 'not set') . "\n";
echo "age param: " . ($_GET['age'] ?? 'not set') . "\n";
echo "\n--- POST ---\n";
echo "name POST: " . ($_POST['name'] ?? 'not set') . "\n";
echo "\n--- ALL ENV ---\n";
foreach ($_SERVER as $k => $v) {
    echo "$k=$v\n";
}
?>