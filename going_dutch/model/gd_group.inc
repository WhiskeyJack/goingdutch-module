<?php

class GD_Group {

  var $GroupId;
  var $Details;
  var $Expenses;
  var $Deposits;
  var $Members;
  var $EventSummary;
  protected $Balance;
  protected $Headers;
  protected $EventHeaders;

  function GD_Group($groupid) {
    $this->fetch_details($groupid);
  }

  /**
   * Retrieves the Going Dutch group details. 
   * Group details are stored in $this->Details as an object.
   */
  function fetch_details($groupid) {
    $this->GroupId = $groupid;
    gd_connect_db();
    $sql = "SELECT `groups`.*, GROUP_CONCAT(`users`.`user_id`) AS member_ids 
                  FROM `users_groups`, `users`, `groups` WHERE 
                  `users_groups`.`user_id` = `users`.`user_id` AND
                  `groups`.`group_id` = `users_groups`.`group_id` AND
                  `users_groups`.`group_id` = {$groupid} GROUP BY `users_groups`.`group_id`";
    $result = db_query($sql);
    $this->Details = $result->fetchObject();
    $this->Details->member_ids = explode(',', $this->Details->member_ids);
    db_set_active();
  }

  /**
   * Fetches details for each of the members in this group. 
   * Stores details as objects in $this->Members with GD user_id as key. 
   * Returns number of users in this group. 
   * @return int 
   */
  function fetch_members() {
    gd_connect_db();
    $query = db_select('users', 'u')->fields('u');
    $query->leftJoin('preferences', 'p', 'u.user_id = p.user_id');
    $query->fields('u', array('user_id', 'drupal_id', 'username', 'email', 'realname', 'reg_date', 'last_login'));
    $query->fields('p', array('name_format', 'email_notify'));
    $query->condition('u.user_id', $this->Details->member_ids, 'IN');
    $query->condition('u.activated', 1, '=');
    $result = $query->execute();
    while ($record = $result->fetchObject())
      $this->Members[$record->user_id] = $record;
    db_set_active();
    return count($this->Members);
  }

  /**
   * Returns the members of this group in an array
   * @return array
   */
  function get_members() {
    if (count($this->Members) == 0)
      $this->fetch_members();

    return $this->Members;
  }

  /**
   * Get total expenses made and paid by each user and calculates the balance
   * Made, paid and balance are added to the user details in $this->Members
   * Member balance overview is stored in $this->Balance
   */
  function calc_user_balance() {
    if (count($this->Members) == 0)
      $this->fetch_members();
    $mergeArr = array();
    gd_connect_db();

    // SUM total of expenses participated in per member and store in $mergeArr
    $sql = "SELECT userid, SUM(total_amount) as grand_total, SUM(per_person) as person_total, COUNT(exid) AS exp_count FROM (
                      SELECT users_expenses.user_id AS userid, group_id, users_expenses.expense_id AS exid, amount as total_amount, 
                        (SELECT COUNT(*) FROM users_expenses WHERE expense_id = exid) as member_count,
                        (SELECT total_amount/member_count) as per_person
                      FROM users_expenses, expenses 
                      WHERE users_expenses.expense_id = expenses.expense_id 
                      AND group_id = {$this->GroupId}
                      AND users_expenses.user_id IN (" . implode(',', $this->Details->member_ids) . ")
                    ) as t1 GROUP BY userid";
    $result = db_query($sql);
    while ($record = $result->fetchAssoc()) {
      $mergeArr[$record['userid']]['expenses_made'] = $record['person_total'];
      $mergeArr[$record['userid']]['expenses_made_count'] = $record['exp_count'];
    }

    // SUM total of expenses paid per member and store in $mergeArr
    $sql = "SELECT user_id AS userid, SUM( amount ) AS expenses_paid, COUNT( expense_id ) AS expenses_paid_count
                  FROM expenses WHERE user_id IN (" . implode(',', $this->Details->member_ids) . ") AND group_id = {$this->GroupId} GROUP BY userid";
    $result = db_query($sql);
    while ($record = $result->fetchAssoc()) {
      $mergeArr[$record['userid']]['expenses_paid'] = $record['expenses_paid'];
      $mergeArr[$record['userid']]['expenses_paid_count'] = $record['expenses_paid_count'];
    }

    // Add expenses paid and made details and total balance per person to $this->Members by merging arrays
    foreach ($this->Members as $userid => $details) {
      if (!isset($mergeArr[$userid]['expenses_made'])) {
        $mergeArr[$userid]['expenses_made'] = 0;
        $mergeArr[$userid]['expenses_made_count'] = 0;
      }
      if (!isset($mergeArr[$userid]['expenses_paid'])) {
        $mergeArr[$userid]['expenses_paid'] = 0;
        $mergeArr[$userid]['expenses_paid_count'] = 0;
      }

      $mergeArr[$userid]['balance'] = $mergeArr[$userid]['expenses_paid'] - $mergeArr[$userid]['expenses_made'];
      $mergeArr[$userid]['formattedname'] = GDme()->format_name($this->Members[$userid]);
//      GDme()->format_name($entry['member'], $userdetails->name_format)),
      $a = 1;
      $this->Members[$userid] = (object) array_merge((array) $this->Members[$userid], $mergeArr[$userid]);
      $mergeArr[$userid]['member'] = (array) $this->Members[$userid];
    }

    // Create balance overview, sorted high to low
    $this->Balance = bs_array_sort($mergeArr, 'balance', SORT_DESC);
    db_set_active();
  }

  /**
   * Returns the balance in an aray for each member in the group, sorted DESC. 
   * Each key is a gd userid, the values contain  expenses_made, expenses_paid 
   * expenses_made_count, expenses_paid_count and balance. 
   * Member details are stored in member with username, realname, email etc. 
   * @return array 
   */
  function get_balance() {
    if (count($this->Balance) == 0)
      $this->calc_user_balance();
    return $this->Balance;
  }

  function get_balance_for_user($gd_userid) {
    if (count($this->Balance) == 0)
      $this->calc_user_balance();
    return $this->Balance[$gd_userid];
  }

  /**
   * Returns the balance list as an array to be used with theme_table
   * Expects number_format as an object defining php number_format options
   * 
   * @return type array for theme_table
   */
  function get_balance_table($userdetails = FALSE) {
    if (count($this->Balance) == 0)
      $this->calc_user_balance();

    $headers = array(
        array('data' => t('Member'), 'field' => 'formattedname'),
        array('data' => t('Total paid'), 'field' => 'expenses_paid', 'class' => array('r_align')),
        array('data' => t('Total spent'), 'field' => 'expenses_made', 'class' => array('r_align')),
        array('data' => t('Balance'), 'field' => 'balance', 'sort' => 'desc', 'class' => array('r_align'))
    );

    $order = tablesort_get_order($headers);
    $sort = tablesort_get_sort($headers);
    $sort = $sort == 'asc' ? SORT_ASC : SORT_DESC;
    $this->Balance = bs_array_sort($this->Balance, $order['sql'], $sort);

    $rows = array();
    foreach ($this->Balance as $entry) {
      $rows[] = array(
          array('data' => GDme()->format_name($entry['member'], $userdetails->name_format)),
          GDme()->cashval_cell($entry['expenses_paid']),
          GDme()->cashval_cell($entry['expenses_made']),
          GDme()->cashval_cell($entry['balance'])
      );
    }
    $attributes = array('class' => array('gd'));
    return array('header' => $headers, 'attributes' => $attributes, 'rows' => $rows);
  }

  /**
   *  Returns an array to be used with theme_table
   *  There is no separate get_expenses as the table is using TableSort and Pager
   *  $userdetails is used to format name and cash values
   * @param type $userdetails
   * @return array  
   */
  function get_expenses_table($userdetails) {
    if (count($this->Expenses == 0))
      $this->get_expenses($userdetails);
    $properties = $this->format_expenses_table($this->Expenses, $this->Headers, $userdetails);

    $table = array('header' => $properties['header'], 'rows' => $properties ['rows'], 'attributes' => $properties['attributes'],
        //'caption' => 'Creating Drupal 7 style tables',
        'sticky' => TRUE,
        'empty' => t('No expenses found')
    );
    return $table;
  }

  function get_expenses($userdetails, $get_all = FALSE) {
    $limit = 20;
    $sort = 'desc';
    if ($get_all) {
      $limit = 10000;
      $sort = 'asc';
    }
    $this->Headers = array(
        array('data' => t('Expense date'), 'field' => 'expense_timestamp', 'sort' => $sort),
        array('data' => t('Description'), 'field' => 'description'),
        array('data' => t('Event'), 'field' => 'event_name'),
        array('data' => t('Buyer'), 'field' => 'formatted_name'),
        array('data' => t('Entry date'), 'field' => 'logged_timestamp'),
        array('data' => t('Persons'), 'field' => 'expense_user_id_count'),
        array('data' => t('Cost pp'), 'field' => 'amount_per_person', 'class' => array('r_align')),
        array('data' => t('Total'), 'field' => 'amount', 'class' => array('r_align'))
    );


    gd_connect_db();
    $query = db_select('expenses', 'e')->extend('PagerDefault')->limit($limit)->extend('TableSort');
    $query->leftJoin('users_expenses', 'ue', 'e.expense_id = ue.expense_id');
    $query->leftJoin('events', 'ev', 'e.event_id = ev.event_id');
    $query->leftJoin('users', 'u', 'e.user_id = u.user_id');
    $query->fields('e');
    $query->fields('ev', array('event_name', 'event_description'));
    $query->addField('ev', 'organizer_id', 'event_organizer_id');
    $query->condition('e.group_id', $this->GroupId, '=');
    $query->isNull('e.deposit_id');
    $query->addExpression('GROUP_CONCAT(ue.user_id)', 'expense_user_ids');
    $query->addExpression(GDme()->format_name_for_dbselect($userdetails->name_format, 'u'), 'formatted_name');
    $query->addExpression('UNIX_TIMESTAMP(e.expense_date)', 'expense_timestamp');
    $query->addExpression('UNIX_TIMESTAMP(e.timestamp)', 'logged_timestamp');
    $query->addExpression('COUNT(ue.user_id)', 'expense_user_id_count');
    $query->addExpression('(amount/COUNT(ue.user_id))', 'amount_per_person');
    $query->groupBy('e.expense_id');
    $query->orderByHeader($this->Headers);
    $this->Expenses = $query->execute()->fetchAllAssoc('expense_id');  // put everything in array so we can close db connection
    db_set_active();
  }

  /**
   *  Returns an array to be used with theme_table
   *  There is no separate get_details as the table is using TableSort and Pager
   *  $userdetails is used to format name and cash values
   * @param type $userdetails
   * @return array  
   */
  function get_deposits_table($userdetails, $get_all=FALSE) {
    $limit = 20;
    $sort = 'desc';
    if ($get_all) {
      $limit = 10000;
      $sort = 'asc';
    }
    $headers = array(
        array('data' => t('Deposit date'), 'field' => 'expense_timestamp', 'sort' => 'desc'),
        array('data' => t('Description'), 'field' => 'description'),
        array('data' => t('Event'), 'field' => 'event_name'),
        array('data' => t('Holder'), 'field' => 'formatted_name'),
        array('data' => t('Entry date'), 'field' => 'logged_timestamp'),
        array('data' => t('Persons'), 'field' => 'expense_user_id_count'),
        array('data' => t('Deposit'), 'field' => 'amount_per_person', 'class' => array('r_align')),
        array('data' => t('Total'), 'field' => 'deposit_total', 'class' => array('r_align'))
    );

    gd_connect_db();
    $query = db_select('expenses', 'e')->extend('PagerDefault')->limit(20)->extend('TableSort');
    $query->leftJoin('users_expenses', 'ue', 'e.expense_id = ue.expense_id');
    $query->leftJoin('events', 'ev', 'e.event_id = ev.event_id');
    $query->leftJoin('deposits', 'd', 'e.deposit_id = d.deposit_id');
    $query->leftJoin('users', 'u', 'd.holder = u.user_id');
    $query->fields('e');
    $query->fields('ev', array('event_name', 'event_description'));
    $query->addField('ev', 'organizer_id', 'event_organizer_id');
    $query->addField('u', 'user_id', 'deposit_holder_id');
    $query->condition('e.group_id', $this->GroupId, '=');
    $query->isNotNull('e.deposit_id');
    $query->addExpression('GROUP_CONCAT(e.user_id)', 'expense_user_ids');
    $query->addExpression(GDme()->format_name_for_dbselect($userdetails->name_format, 'u'), 'formatted_name');
    $query->addExpression('UNIX_TIMESTAMP(e.expense_date)', 'expense_timestamp');
    $query->addExpression('UNIX_TIMESTAMP(e.timestamp)', 'logged_timestamp');
    $query->addExpression('SUM(e.amount)', 'deposit_total');
    $query->addExpression('COUNT(e.user_id)', 'expense_user_id_count');
    $query->addExpression('(SUM(e.amount)/COUNT(e.user_id))', 'amount_per_person');
    $query->groupBy('e.deposit_id');
    $query->orderByHeader($headers);
    // todo: fix the user ids coming back for a deposit
    $string = $query->__toString();
    $this->Deposits = $query->execute()->fetchAllAssoc('deposit_id');  // put everything in array so we can close db connection
    db_set_active();

    if ($get_all)
      return;

    $properties = $this->format_expenses_table($this->Deposits, $headers, $userdetails);

    $table = array('header' => $properties['header'], 'rows' => $properties ['rows'], 'attributes' => $properties['attributes'],
        //'caption' => 'Creating Drupal 7 style tables',
        'sticky' => TRUE,
        'empty' => t('No deposits found')
    );
    return $table;
  }

  function format_expenses_table($resultArr, $headers, $userdetails) {
    $rows = array();
    foreach ($resultArr as $expense_id => $row) {
      $fields = array();
      foreach ($headers as $col) {
        if ($col['field'] == 'expense_timestamp' || $col['field'] == 'logged_timestamp') {
          $fields[$col['field']] = format_date($row->$col['field'], 'gd_expense_date');
        } elseif ($col['field'] == 'amount' || $col['field'] == 'amount_per_person' || $col['field'] == 'deposit_total')
          $fields[$col['field']] = GDme()->cashval_cell($row->$col['field']);
        elseif ($col['field'] == 'expense_user_id_count')
          $fields[$col['field']] = array('data' => $row->$col['field'], 'class' => array('c_align'));
        else
          $fields[$col['field']] = $row->$col['field'];
      }
      $userlist = explode(',', $row->expense_user_ids);
      if($row->type == 3 && !in_array($userdetails->user_id, $userlist)) {
        // present for this user or that this user did not take part in
         $fields['description'] = t('-- present --' );
         $fields['formatted_name'] = t('-- present --' );
         $fields['logged_timestamp'] = t('--' );
         $fields['expense_user_id_count'] = array('data' => t('--'), 'class' => array('c_align'));
         $fields['amount_per_person'] = array('data' => t('--'), 'class' => array('cash_value'));
         $fields['amount'] = array('data' => t('--'), 'class' => array('cash_value'));
      }
      $rows[] = array('data' => $fields);
    }
    $attributes = array('class' => array('gd'));
    $properties = array('header' => $headers, 'rows' => $rows, 'attributes' => $attributes);
    return $properties;
  }

    function format_events_summary_table($resultArr, $headers, $userdetails) {
    $rows = array();
    foreach ($resultArr as $event_id => $row) {
      $fields = array();
      foreach ($headers as $col) {
        if ($col['field'] == 'event_timestamp') {
          $fields[$col['field']] = format_date($row->$col['field'], 'gd_expense_date');
        } elseif ($col['field'] == 'total_amount' || $col['field'] == 'avg_amount_per_expense')
          $fields[$col['field']] = GDme()->cashval_cell($row->$col['field']);
        elseif ($col['field'] == 'expense_count')
          $fields[$col['field']] = array('data' => $row->$col['field'], 'class' => array('c_align'));
        else
          $fields[$col['field']] = $row->$col['field'];
      }
      $rows[] = array('data' => $fields);
    }
    $attributes = array('class' => array('gd'));
    $properties = array('header' => $headers, 'rows' => $rows, 'attributes' => $attributes);
    return $properties;
  }
  
  
    function get_events_summary_table($userdetails) {
      $a=1;
    if (count($this->EventSummary == 0))
      $this->get_events_summary();
    
    $properties = $this->format_events_summary_table($this->EventSummary, $this->EventHeaders, $userdetails);

    $table = array('header' => $properties['header'], 'rows' => $properties ['rows'], 'attributes' => $properties['attributes'],
        //'caption' => 'Creating Drupal 7 style tables',
        'sticky' => TRUE,
        'empty' => t('No expenses found')
    );
    return $table;
  }
  
/**
 * Returns a list of events for the specified time period
 * $from and $to should be iso dates (yyyy-mm-dd)
 * Dates are inclusive
 * When $from or $to is empty, it is not used as a restraint
 * @param type $from
 * @param type $to 
 */
  function get_event_list($from = '', $to = ''){
    gd_connect_db();
    $query = db_select('events', 'e')->fields('e');
    $query->condition('e.group_id', $this->GroupId, '=');
    if (!empty($from)) $query->condition('e.date', $from, '>=');
    if (!empty($to)) $query->condition('e.date', $to, '<=');
    $query->addExpression('UNIX_TIMESTAMP(e.date)', 'event_timestamp');
    $eventList = $query->execute()->fetchAllAssoc('event_id'); 
    db_set_active();
    return $eventList;
  }
  
  
   function get_events_summary() {

    $this->EventHeaders = array(
        array('data' => t('Event date'), 'field' => 'event_timestamp', 'sort' => 'desc'),
        array('data' => t('Event'), 'field' => 'event_name'),
        array('data' => t('Description'), 'field' => 'event_description'),
        array('data' => t('Expenses'), 'field' => 'expense_count', 'class' => array('c_align')),
        array('data' => t('Average'), 'field' => 'avg_amount_per_expense', 'class' => array('r_align')),
        array('data' => t('Total'), 'field' => 'total_amount', 'class' => array('r_align'))
    );

    gd_connect_db();
    $query = db_select('events', 'ev')->extend('PagerDefault')->limit(20)->extend('TableSort');
    //$query->leftJoin('users_expenses', 'ue', 'e.expense_id = ue.expense_id');
    $query->leftJoin('expenses', 'e', 'ev.event_id = e.event_id');
    //$query->leftJoin('users', 'u', 'e.user_id = u.user_id');
    $query->fields('e', array('event_id', 'group_id'));
    $query->fields('ev', array('event_name', 'event_description'));
    $query->addField('ev', 'organizer_id', 'event_organizer_id');
    $query->condition('ev.group_id', $this->GroupId, '=');
    //$query->condition('e.event_id', 0, '>');
    //$query->isNull('e.deposit_id');
    //$query->addExpression('GROUP_CONCAT(ue.user_id)', 'expense_user_ids');
    //$query->addExpression(GDme()->format_name_for_dbselect($userdetails->name_format, 'u'), 'formatted_name');
    //$query->addExpression('UNIX_TIMESTAMP(e.expense_date)', 'expense_timestamp');
    $query->addExpression('UNIX_TIMESTAMP(ev.date)', 'event_timestamp');
    $query->addExpression('COUNT(e.expense_id)', 'expense_count');
    $query->addExpression('SUM(e.amount)', 'total_amount');
    $query->addExpression('(SUM(amount)/COUNT(e.expense_id))', 'avg_amount_per_expense');    
    $query->groupBy('ev.event_id');
    $query->orderByHeader($this->EventHeaders);
    
    /*
        $query = db_select('expenses', 'e')->extend('PagerDefault')->limit(20)->extend('TableSort');
    //$query->leftJoin('users_expenses', 'ue', 'e.expense_id = ue.expense_id');
    $query->leftJoin('events', 'ev', 'e.event_id = ev.event_id');
    //$query->leftJoin('users', 'u', 'e.user_id = u.user_id');
    $query->fields('e', array('event_id', 'group_id'));
    $query->fields('ev', array('event_name', 'event_description'));
    $query->addField('ev', 'organizer_id', 'event_organizer_id');
    $query->condition('e.group_id', $this->GroupId, '=');
    $query->condition('e.event_id', 0, '>');
    $query->isNull('e.deposit_id');
    //$query->addExpression('GROUP_CONCAT(ue.user_id)', 'expense_user_ids');
    //$query->addExpression(GDme()->format_name_for_dbselect($userdetails->name_format, 'u'), 'formatted_name');
    //$query->addExpression('UNIX_TIMESTAMP(e.expense_date)', 'expense_timestamp');
    $query->addExpression('UNIX_TIMESTAMP(ev.date)', 'event_timestamp');
    $query->addExpression('COUNT(e.expense_id)', 'expense_count');
    $query->addExpression('SUM(e.amount)', 'total_amount');
    $query->addExpression('(SUM(amount)/COUNT(e.expense_id))', 'avg_amount_per_expense');    
    $query->groupBy('e.event_id');
    $query->orderByHeader($this->EventHeaders);
    */
    
    //$str = $query->__toString();

   $this->EventSummary = $query->execute()->fetchAllAssoc('event_id');  // put everything in array so we can close db connection
    db_set_active();
    
  }
  
  /**
   * Returns a value formatted with php number_format. 
   * Only serves as an alias for number_format to keep code readable
   * @param type $val value to be formatted
   * @param type $numberformat as object with decimal_count, decimal_sep and thousand_sep
   */
  function fn($val, $numberformat) {
    if (!$numberformat) {
      $nf = new stdClass;
      $nf->thousand_sep = '.';
      $nf->decimal_sep = ',';
      $nf->decimal_count = 2;
      $nf->currency_symbol = '&#8364;';
      $nf->currency_location = 'before';
    } else
      $nf = $numberformat;

    if ($nf->currency_location == 'before')
      $fval = $nf->currency_symbol . ' ' . number_format($val, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep);
    elseif ($nf->currency_location == 'after')
      $fval = number_format($val, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep) . ' ' . $nf->currency_symbol;
    else
      $fval = number_format($val, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep);
    return $fval;
  }

}
