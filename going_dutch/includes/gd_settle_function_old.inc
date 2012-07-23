<?php

function close_group_expenses($listarray) {
  // http://stackoverflow.com/questions/877728/what-algorithm-to-use-to-determine-minimum-number-of-actions-required-to-get-the/
  // remove members that have a zero balance
  foreach ($listarray as $key => $val) {
    if ($val['balance'] == 0) {
      //echo $val['name'] . "<br>";
      unset($listarray[$key]);
    }
  }

  $listarraycp = $listarray;
  //printr($listarray);
  /* $listarray = array (
    0 => array("uid" => 1, "balance" => "10"),
    1 => array("uid" => 2, "balance" => "-5"),
    2 => array("uid" => 3, "balance" => "8.5"),
    3 => array("uid" => 4, "balance" => "-20"),
    4 => array("uid" => 5, "balance" => "25"),
    5 => array("uid" => 6, "balance" => "-10"),
    6 => array("uid" => 7, "balance" => "-5.5"),
    7 => array("uid" => 8, "balance" => "-2"),
    8 => array("uid" => 9, "balance" => "-1")
    ); */


  $i = 0;
  $stop = false;

  do {

    $texpense_array = false;
    $texpense_array = check_even_balance($listarray);
    if ($texpense_array) {
      $expense_array[$i] = $texpense_array;
      //printr($expense_array);
      // TODO: add loop again if succesful
      ////echo "key $key and key $key2 are even balance";
      unset($listarray[$expense_array[$i]['pay_key']]);
      unset($listarray[$expense_array[$i]['get_key']]);
      $i++;
    }
  } while ($texpense_array);

  // echo "LISTARRAY AFTER FIRST ROUND<br>";
  //printr($listarray);

  if (count($listarray) > 0) {
    // check if wiping one balance can wipe two in the next round
    do {
      $texpense_array = false;
      $texpense_array = check_even_balance_next_round($listarray);
      if ($texpense_array) {
        // $expense_array[] = $texpense_array;
        //printr($texpense_array);
        $expense_array[$i] = $texpense_array[0];
        unset($listarray[$texpense_array[0]['get_key']]);
        $expense_array[$i + 1] = $texpense_array[1];
        unset($listarray[$texpense_array[1]['pay_key']]);
        unset($listarray[$texpense_array[1]['get_key']]);
        $i = $i + 2;
        //printr($expense_array);
        //unset($listarray[$expense_array[$i]['pay_key']]);
        // TODO: add loop again if succesful
        ////echo "key $key and key $key2 are even balance";
        // unset($listarray[$expense_array[$i]['pay_key']]);
        // unset($listarray[$expense_array[$i]['get_key']]);
        // $i++;
      }
    } while ($texpense_array);
  }

  // echo "LISTARRAY AFTER SECOND ROUND<br>";
  //printr($listarray);

  do {
    // wipe any balance
    // kleinst mogelijke negatieve eerst met kleinst mogelijke positieve
    // echo get_memory_usage() . "<br>";
    if (count($listarray) > 0) {
      $lowbalance = 0;
      foreach ($listarray as $key => $value) {


        // find largest negative and positive balance
        //echo "($lowbalance > ". $value['balance'] . "&& " . $value['balance'] . " < 0 )<br>";
        if ((!isset($lowkey) && $value['balance'] < 0) || ($lowbalance < $value['balance'] && $value['balance'] < 0 )) {

          $lowkey = $key;
          $lowbalance = $value['balance'];
          //echo "lowbalance = $lowbalance<br>";
        }
      }
      $highbalance = 0;
      foreach ($listarray as $key => $value) {

        if ((!isset($highkey) && $value['balance'] > 0) || ($highbalance > $value['balance'] && $value['balance'] > 0 && ($value['balance'] > abs($lowbalance) || ( $value['balance'] > $highbalance && $highbalance < abs($lowbalance))) )) {
          $highkey = $key;
          $highbalance = $value['balance'];
        }
      }
    }

    if ($highbalance > abs($lowbalance)) {
      //$debugstr = "high > low: " .$listarray[$lowkey]['name'] . " (". $listarray[$lowkey]['balance'] . ", key: $lowkey) pays " . abs($lowbalance) ." to " . $listarray[$highkey]['name'] . " (". $listarray[$highkey]['balance'] . ", key: $highkey)<br>";
      $expense_array[$i]['pay_uid'] = $listarray[$lowkey]['user_id'];
      $expense_array[$i]['pay_name'] = $listarray[$lowkey]['name'];
      $expense_array[$i]['pay_key'] = $lowkey;
      $expense_array[$i]['sum'] = abs($lowbalance);
      unset($listarray[$lowkey]);
      $expense_array[$i]['get_uid'] = $listarray[$highkey]['user_id'];
      $expense_array[$i]['get_name'] = $listarray[$highkey]['name'];
      $expense_array[$i]['get_key'] = $highkey;
      $listarray[$highkey]['balance'] += $lowbalance;
    } else {
      //$debugstr = "low > high: " .$listarray[$lowkey]['name'] . " (". $listarray[$lowkey]['balance'] . ", key: $lowkey) pays ". abs($highbalance) . " to " . $listarray[$highkey]['name'] . " (". $listarray[$highkey]['balance'] . ", key: $highkey)<br>";
      $expense_array[$i]['pay_uid'] = $listarray[$lowkey]['user_id'];
      $expense_array[$i]['pay_name'] = $listarray[$lowkey]['name'];
      $expense_array[$i]['pay_key'] = $lowkey;
      $expense_array[$i]['sum'] = $highbalance;

      $expense_array[$i]['get_uid'] = $listarray[$highkey]['user_id'];
      $expense_array[$i]['get_name'] = $listarray[$highkey]['name'];
      $expense_array[$i]['get_key'] = $highkey;
      unset($listarray[$highkey]);
      $listarray[$lowkey]['balance'] += $highbalance;
    }

    $i++;
    //echo "LOWKEY: $lowkey, LOWBALANCE: $lowbalance, HIGHKEY: $highkey, HIGHBALANCE: $highbalance<br>";

    unset($highkey);
    unset($lowkey);
    unset($highbalance);
    unset($lowbalance);
    //echo "LISTARRAY ===<br>";
    //printr($listarray);
    // add loop after last wipe until count $listarray == 0
    //echo "Expense array  AFTER THIRD ROUND<br>";
    //printr($expense_array);
    /////////////////
    if (count($listarray) > 0) {
      // check if wiping one balance can wipe two in the next round
      do {
        $texpense_array = false;
        $texpense_array = check_even_balance_next_round($listarray);
        if ($texpense_array) {
          // $expense_array[] = $texpense_array;
          //printr($texpense_array);
          $expense_array[$i] = $texpense_array[0];
          unset($listarray[$texpense_array[0]['get_key']]);
          $expense_array[$i + 1] = $texpense_array[1];
          unset($listarray[$texpense_array[1]['pay_key']]);
          unset($listarray[$texpense_array[1]['get_key']]);
          $i = $i + 2;
          //printr($expense_array);
          //unset($listarray[$expense_array[$i]['pay_key']]);
          // TODO: add loop again if succesful
          ////echo "key $key and key $key2 are even balance";
          // unset($listarray[$expense_array[$i]['pay_key']]);
          // unset($listarray[$expense_array[$i]['get_key']]);
          // $i++;
        }
      } while ($texpense_array);
    }

    //echo "LISTARRAY AFTER FOURTH ROUND<br>";
    //printr($listarray);
  } while (count($listarray) > 1);


  // printr($expense_array);
  /* echo "To even out the balances: <br><br>";
    foreach ($expense_array as $key => $value) {
    echo $listarraycp[$value['pay_key']]['name'] . " pays &euro; " . number_format($value['sum'], DECIMALS , DSEP,TSEP)  . " to " . $listarraycp[$value['get_key']]['name'] . "<br>";
    } */
  return ($expense_array);
}


function check_even_balance($listarray) {
  // check if one balance can wipe the other
  $expense_array = false;
  $stop = false;
  foreach ($listarray as $key => $value) {
    foreach ($listarray as $key2 => $value2) {
      ////echo "$key:$key2 - " . $value['balance'] ." == " .  $value2['balance'] ."<br>";
      if ($key != $key2 && $value['balance'] == -$value2['balance']) {
        //The expression (expr1) ? (expr2) : (expr3) evaluates to expr2 if expr1 evaluates to TRUE, and expr3 if expr1 evaluates to FALSE. 
        if ($value['balance'] > $value2['balance']) {
          $expense_array['pay_uid'] = $value2['user_id'];
          $expense_array['pay_name'] = $value2['name'];
          $expense_array['pay_key'] = $key2;
          $expense_array['get_uid'] = $value['user_id'];
          $expense_array['get_name'] = $value['name'];
          $expense_array['get_key'] = $key;
        } else {
          $expense_array['pay_uid'] = $value['user_id'];
          $expense_array['pay_name'] = $value['name'];
          $expense_array['pay_key'] = $key;
          $expense_array['get_uid'] = $value2['user_id'];
          $expense_array['get_name'] = $value2['name'];
          $expense_array['get_key'] = $key2;
        }

        $expense_array['sum'] = abs($value['balance']);
        ////echo "key $key and key $key2 are even balance";
        unset($listarray[$key]);
        unset($listarray[$key2]);
        $i++;
        $stop = true;
        break;
      }
    }
    if ($stop)
      break;
  }
  return $expense_array;
}

function check_even_balance_next_round($listarray) {
  $lastkey = false;
  $stop = false;
  foreach ($listarray as $key => $value) {
    //$temparray = $listarray;

    if ($lastkey) {
      $temparray = $listarray;

      foreach ($temparray as $key3 => $value3) {
        if ($key3 != $lastkey) {
          ($listarray[$lastkey]['balance'] < 0) ? $temparray[$key3]['balance'] -= $listarray[$lastkey]['balance'] : $temparray[$key3]['balance'] += $listarray[$lastkey]['balance'];
          unset($temparray[$lastkey]);
          // //echo "Checking KEY: $key, UID: ".$listarray[$key]['uid'].", balance of UID " .$listarray[$lastkey]['uid'] . " (key $lastkey) set to zero (balanced with uid " . $temparray[$key3]['uid'] ." (key $key3)<br>";
          // //echo "Running through this temparray: ";
          ////printr($temparray);
          $expense_array2[1] = check_even_balance($temparray);
          if ($expense_array2[1] != false) {
            if ($listarray[$lastkey]['balance'] > $temparray[$key3]['balance']) {
              $expense_array2[0]['pay_uid'] = $temparray[$key3]['user_id'];
              $expense_array2[0]['pay_name'] = $temparray[$key3]['name'];
              $expense_array2[0]['pay_key'] = $key3;
              $expense_array2[0]['get_uid'] = $listarray[$lastkey]['user_id'];
              $expense_array2[0]['get_name'] = $listarray[$lastkey]['name'];
              $expense_array2[0]['get_key'] = $lastkey;
            } else {
              $expense_array2[0]['pay_uid'] = $listarray[$lastkey]['user_id'];
              $expense_array2[0]['name'] = $listarray[$lastkey]['name'];
              $expense_array2[0]['pay_key'] = $lastkey;
              $expense_array2[0]['get_uid'] = $temparray[$key3]['user_id'];
              $expense_array2[0]['get_name'] = $temparray[$key3]['name'];
              $expense_array2[0]['get_key'] = $key3;
            }
            $expense_array2[0]['sum'] = abs($listarray[$lastkey]['balance']);
            //echo "FOUND<br>";
            ////printr($expense_array2);
            $return_array[] = $expense_array2;
            //$stop = true;
            //return ($expense_array2);
            //break;
          }

          //foreach ($temparray as $key2 => $value2) {
          //}
          //reset temp array
          ($listarray[$lastkey]['balance'] < 0) ? $temparray[$key3]['balance'] += $listarray[$lastkey]['balance'] : $temparray[$key3]['balance'] -= $listarray[$lastkey]['balance'];
        }
        //if ($stop) break;
      }
    }
    $lastkey = $key;

    //if ($stop) break;
  }
  // find transactions with lowest total sum
  $selected = 0;
  if (isset($return_array)) {
    foreach ($return_array as $key => $value) {
      if ($sum == false || $sum > $value[0]['sum'] + $value[1]['sum']) {
        $selected = $key;
        $sum = $value[0]['sum'] + $value[1]['sum'];
      }
    }
    return ($return_array[$selected]);
  }
//echo "=========";
//printr($return_array[$selected]);
//echo "=========+";
}
