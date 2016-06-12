<?php
	error_reporting(E_ALL & ~E_NOTICE);
	//error_reporting(0); // disable error reporting
	$fval_path = "/tmp/am2321-val.txt";
	$flog_path = "/tmp/am2321-log.txt";
	// some setup
	$sensor_name = "SERVER ROOM";
	$correction_humi = 0.0;
	$correction_temp = 0.0;
	//$timezone = date_default_timezone_get(); // use system wide timezone (check http://php.net/manual/en/function.date-default-timezone-get.php for more info)
	$timezone = "GMT+2";
	
	
	
	// adjust corrections to 0.1 resolution
	$correction_humi = $correction_humi*10;
	$correction_temp = $correction_temp*10;
	
	$val1 = file($fval_path,  FILE_IGNORE_NEW_LINES |  FILE_SKIP_EMPTY_LINES );
	//echo "<b>val1</b>: <pre>".print_r($val1,true)."</pre><br/>";
	$val = array("x");
	foreach($val1 as $ln) { 
		//echo "Line: <code>$ln</code><br/>";
		// break each line into array
		$val2 = explode(":",$ln);
		//echo "<b>val2</b>: <pre>".print_r($val2,true)."</pre><br/>";
		if (count($val2) == 2) {
			if ($val2[0] == "v") {
				unset($val);
				$val = array_merge(array("v"), explode(";",$val2[1]));
			}
		}
		//echo "<hr/>";
	}
	//echo "<b>val</b>: <pre>".print_r($val,true)."</pre><br/>";

	$res = 0;
	$humi = 1010;
	$temp = 1250;
	$time = new DateTime();
	$time->setTimezone(new DateTimeZone($timezone));
	//$time->setTimezone(new DateTimeZone()); 
	
	if ($val[0] == "v" && count($val) >= 8) {
		$t = intval($val[1],10);
		$time->setTimestamp($t);
		if ($val[2] == "rv") {
			if ($val[3] == 0) {
				$res=1;
				$humi = intval(intval($val[4],16)+$correction_humi);
				$temp = intval(intval($val[5],16)+$correction_temp);
			} else {
				$res=0;
			}
		}
	}
	
	// check if xml requset
	if (isset($_GET["xml"])) {
		header("Content-type: text/xml");
		$xml = '<?xml version="1.0" encoding="UTF-8"?>'."\n";
		$xml .= '<sensor><name>'.$sensor_name.'</name>';
		$xml .= "<status>".$res."</status>";
		$xml .= "<time>";
		$xml .= "<iso8601>".$time->format("c")."</iso8601>";
		$xml .= '<unix timezone="UTC">'.$time->getTimestamp()."</unix>";
		$xml .= "</time>";
		$xml .= '<values resolution="0.1">';
		$xml .= "<temp>".$temp."</temp>";
		$xml .= "<humi>".$humi."</humi>";
		$xml .= "</values>";
		$xml .= "</sensor>";
		die($xml);
	}
	
	
?><!DOCTYPE html>
<html lang="pl">
<head>
<title><?php echo $sensor_name; ?> - AM2321 sensor</title>
<style type="text/css">

body {
	font-size: 12pt;
}

h1 {
	text-align: left;
	size: 2em;
	border-bottom: 2px groove;
}

.block {
	width: 400px;
}

.line {
	width: 90%;
}

.line-h {
	display: inline-block;
	font-size: 1.1em;
	font-weight: bold;
	min-width: 40%;
}

.line-v {
	display: inline-block;
}

.line-err {
	font-size: 1.5em;
	font-weight: bold;
	color: #bf0000;
	text-align: center;
}

.line-err-log {
	font-size: 8pt;
	font-family: monospace;
}

</style>
</head>
<body>
<?php 
	
?>
<div class="block">
	<h1><?php echo $sensor_name; ?></h1>
<?php if ($res == 1 && $humi<1001 && $temp<1200) { ?>
	<div class="line"><div class="line-h">Sample time: </div><div class="line-v"><?php echo $time->format("Y F j H:i:s P");
	?></div></div>
	<div class="line"><div class="line-h">Temperature: </div><div class="line-v"><?php echo sprintf("%.1f",$temp/10.0);?>&deg;C</div></div>
	<div class="line"><div class="line-h">Humidity: </div><div class="line-v"><?php echo sprintf("%.1f",$humi/10.0);?>% Rh</div></div>
<?php } else { ?>
	<div class="line"><div class="line-err">Read error. Log:</div></div>
	<div class="line"><div class="line-err-log"><pre><?php echo file_get_contents($flog_path); ?></pre></div></div>
<?php } ?>
</div>
</body>
</html>