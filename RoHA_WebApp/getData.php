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


$response = $dynamodb->scan(array(
    'TableName' => 'wx_data'
));

//print_r ($response['Items']);
$latest = "";
$coughCount = 0;
$healthyCount = 0;

//set the latest date from the first record fetched
$latestdate =  $response['Items'][0]['sample_time']['N'];

//iterate through all data 
foreach ($response['Items'] as $item) {
    //check if latest date is < current date fetched
    //this step will finally get the latest date and status from database
    if($latestdate < $item['sample_time']['N']){
        $latestdate = $item['sample_time']['N'];
        $latest = $item['device_data']['M']['status']['S'];
    }
    //calculating the cough and healty breath count   
    if ($item['device_data']['M']['status']['S'] == "cough"){
        $coughCount++;
    } else if ($item['device_data']['M']['status']['S'] == "healthy breath"){
        $healthyCount++;
    }
}


if($_GET['id']=="cough"){  echo $coughCount; }
if($_GET['id']=="healthy"){ echo $healthyCount;  }
if($_GET['id']=="latest"){ echo $latest;  }
if($_GET['id']=="latestdate"){ echo "<script>$('#latestdate').html(new Date(".$latestdate.").toString());</script>";  }

?>