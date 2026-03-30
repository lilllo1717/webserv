<?php
header('Content-Type: text/html; charset=UTF-8');
?>
<!DOCTYPE html>
<html>
<head><title>CGI Test</title></head>
<body>
    <h1>CGI Works! 🎉</h1>
    <p>POST data: <?php echo htmlspecialchars($_POST['name'] ?? 'none'); ?></p>
    <script>console.log('CGI JavaScript works!');</script>
</body>
</html>
