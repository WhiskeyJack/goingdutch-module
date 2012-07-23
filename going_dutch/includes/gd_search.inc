<?php

function gd_search($searchterm) {
  $html = '<h2>'.t('Search results for @searchterm', array('@searchterm' => $searchterm)) .'</h2>';
  $groups = GDme()->get_groups();
  $groupIds = implode(',', array_keys($groups));
  $sql  = "SELECT `group_id`, `expense_id` , NULL as `event_id`, NULL as `event_name`, `description` , UNIX_TIMESTAMP(`expense_date`) AS `date`, `amount`,
                    MATCH (`description`) AGAINST ('{$searchterm}') AS 'matched'
                    FROM `expenses` WHERE MATCH (`description`) AGAINST ('{$searchterm}') >0
                    AND `group_id` IN ({$groupIds})
                    UNION 
                    SELECT `group_id`, `expense_id` , NULL as `event_id`, NULL as `event_name`, `description` , UNIX_TIMESTAMP(`expense_date`) AS `date`, `amount`,
                    MATCH (`description`) AGAINST ('{$searchterm}*' IN BOOLEAN MODE) AS 'matched'
                    FROM `expenses` WHERE MATCH (`description`) AGAINST ('{$searchterm}*' IN BOOLEAN MODE) >0
                    AND `group_id` IN ({$groupIds})
                    UNION
                    SELECT `group_id`, NULL as `expense_id`, `event_id` , `event_name` , `event_description` as `description`, UNIX_TIMESTAMP(`date`) AS `date`, NULL AS  `amount`,
                    MATCH (`event_name` , `event_description`) AGAINST ('{$searchterm}') AS 'matched'
                    FROM `events` WHERE MATCH (`event_name` , `event_description`) AGAINST ('{$searchterm}')  >0
                    AND `group_id` IN ({$groupIds})
                    UNION
                    SELECT `group_id`, NULL as `expense_id`, `event_id` , `event_name` , `event_description` as `description`, UNIX_TIMESTAMP(`date`) AS `date`, NULL AS  `amount`,
                    MATCH (`event_name` , `event_description`) AGAINST ('{$searchterm}*' IN BOOLEAN MODE) AS 'matched'
                    FROM `events` WHERE MATCH (`event_name` , `event_description`) AGAINST ('{$searchterm}*' IN BOOLEAN MODE)  >0
                    AND `group_id` IN ({$groupIds})
                    ORDER BY matched DESC
                    LIMIT 0 , 100";
    gd_connect_db();
    $result = db_query($sql);
    while ($record = $result->fetchAssoc()) {
      $found[] = $record;
    }
    db_set_active();
    $expenses = array();
    $events = array();
    foreach ($found as $result){
      $html .= '<p>';
      if (isset($result['expense_id']) && !in_array($result['expense_id'], $expenses)){
        $html .= t('Group @groupname: Expense "@expense_description" on !date for !amount', 
                array('@groupname' => $groups[$result['group_id']]->name, '@expense_description' => $result['description'], '!date' => format_date($result['date'], 'long'), '!amount' => GDme()->cashval($result['amount'])));
        $expenses[] = $result['expense_id'];
      }
        if (isset($result['event_id']) && !in_array($result['events_id'], events)){
        $html .= t('Group @groupname: Event "@event_name" with description "@event_description" on !date', 
                array('@groupname' => $groups[$result['group_id']]->name, '@event_name' => $result['event_name'], '@event_description' => $result['description'], '!date' => format_date($result['date'], 'long')));
        $events[] = $result['event_id'];
        }
      $html .= '</p>';  
    }

    return $html;
}