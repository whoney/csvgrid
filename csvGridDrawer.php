#!/usr/bin/php
<?php
/**
 * Created by PhpStorm.
 * User: mrbond
 * Date: 23/03/2017
 * Time: 20:44
 */

if ($argc != 9) {

    echo "Usage: csvGridDrawer.php <csv file> <lon_west> <lon_east> <lat_south> <lat_north> <d_lon> <d_lat> <output_image_path>\n";
}

$inputCsv = $argv[1];
$lon1 = floatval($argv[2]);
$lon2 = floatval($argv[3]);
$lat1 = floatval($argv[4]);
$lat2 = floatval($argv[5]);
$dx = floatval($argv[6]);
$dy = floatval($argv[7]);
$outputImagePath = $argv[8];

$lon1 += $dx;

$fd = @fopen($inputCsv, "r");
if (!$fd) {
    echo "Failed to open file '" . $inputCsv . "'\n";
    die();
}

$width = ($lon2 - $lon1) / $dx + 1;
$height = ($lat2 - $lat1) / $dy + 1;

echo "Creating image " . $width . "x" . $height . "\n";
$im = imagecreatetruecolor($width, $height);

$values = array();

while(!feof($fd)) {

    $ar = fgetcsv($fd);

    $lon = $ar[0] - 180.0; // У тебя lat и lon идут от 0 а не от -180 и -90
    $lat = $ar[1] - 90.0; //
    $val = $ar[2];

    $values[] = array(
        $lon, $lat, $val
    );
}

$result = array();

foreach ($values as $value) {

    $lon = $value[0];
    $lat = $value[1];
    $val = $value[2];

    $x = ($lon - $lon1) / $dx;
    $y = $height - ($lat - $lat1) / $dy;

    $red = $val / 40.0 * 256;

    $color = imagecolorallocate($im, $red, 0, 0);

    imagesetpixel($im, $x, $y, $color);
}

echo "Image save to " . $outputImagePath . "\n";
imagepng($im, $outputImagePath);