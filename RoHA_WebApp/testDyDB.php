<?php

require 'vendor/autoload.php';

date_default_timezone_set('UTC');

use Aws\DynamoDb\Exception\DynamoDbException;
use Aws\DynamoDb\Marshaler;
$credentials = new Aws\Credentials\Credentials('YOUR_ACCESS_KEY_ID', 'YOUR_SECRET_KEY');
$sdk = new Aws\Sdk([
    
    'region'   => 'us-east-1',
    'version'  => 'latest',
    'credentials' => $credentials
]);

$dynamodb = $sdk->createDynamoDb();

/*$response = $dynamodb->query([
    'TableName' => 'wx_data',

    'KeyConditionExpression' => 'sample_time = :sample_time and device_id = :device_id',
    'ExpressionAttributeValues' =>  [
        ':sample_time' => ['N' => '1626616493278'],
        ':device_id' => ['N' => '22']
    ]
]);

*/

$response = $dynamodb->scan(array(
    'TableName' => 'wx_data'
    /*'Limit'=> 1*/
));

//print_r ($response['Items']);
$latest = "";
$coughCount = 0;
$healthyCount = 0;

$atestdate =  $response['Items'][0]['sample_time']['N'];


foreach ($response['Items'] as $item) {
    if($atestdate < $item['sample_time']['N']){
        $atestdate = $item['sample_time']['N'];
        $latest = $item['device_data']['M']['status']['S'];
    }
    echo "Device ID: ";
    echo $item['device_id']['N'] . " <script>document.write(new Date(" . $item['sample_time']['N'] . "));</script> Status: ";
    // Grab the error string value
    echo $item['device_data']['M']['status']['S'] . "<BR>";
    if ($item['device_data']['M']['status']['S'] == "cough"){
        $coughCount++;
    } else if ($item['device_data']['M']['status']['S'] == "healthy breath"){
        $healthyCount++;
    }
}



echo "Cough Count: " . $coughCount . "<br>Healthy Count: " . $healthyCount . "<br>Result: " . $latest . "<br> <script>document.write(new Date(" . $atestdate . "));</script>";



?>