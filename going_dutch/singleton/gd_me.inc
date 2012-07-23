<?php

class GD_Me extends GD_User {

  var $AllGroupBalance;
  protected $ExpenseTypes;

  function GD_Me($userid, $useridtype='drupal') {
    global $user;
    $this->GD_User($userid, $useridtype);
    $user->gd = $this->Details;
    $this->check_for_new_groups();
  }

  /**
   * Formats a table cell for display of a monetary value
   * @param type $val to be formatted
   * @param type $numberformat as object with decimal_count, decimal_sep and thousand_sep
   * @return type array for use in table array as a cell with keys data and class
   */
  function cashval_cell($val, $link = FALSE) {
    if (!$link)
      $cash = $this->fn($val);
    else
      $cash = l($this->fn($val), $link, array('html' => TRUE));
    return array('data' => $cash, 'class' => array('cash_value'));
  }

  /**
   * Formats a value for display as a monetary value
   * @param type $val to be formatted
   * @param type $numberformat as object with decimal_count, decimal_sep and thousand_sep
   * @return type array for use in table array as a cell with keys data and class
   */
  function cashval($val) {
    return $this->fn($val);
  }

  /**
   * Returns a value formatted with php number_format. 
   * Only serves as an alias for number_format to keep code readable
   * @param type $val value to be formatted
   * @param type $numberformat as object with decimal_count, decimal_sep and thousand_sep
   */
  function fn($val) {
    if (!$this->Details->number_format) {
      $nf = new stdClass;
      $nf->thousand_sep = '.';
      $nf->decimal_sep = ',';
      $nf->decimal_count = 2;
      $nf->currency_symbol = '&#8364;';
      $nf->currency_location = 'before';
    } else
      $nf = $this->Details->number_format;

    if ($nf->currency_location == 'before')
      $fval = $nf->currency_symbol . ' ' . number_format($val, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep);
    elseif ($nf->currency_location == 'after')
      $fval = number_format($val, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep) . ' ' . $nf->currency_symbol;
    else
      $fval = number_format($val, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep);
    return $fval;
  }

  /**
   * Returns a formatted name according to the names listed in $formatArray. 
   * Only a combination of realname and username is possible, so max two 
   * values are expected in $formatArray
   * @param array $nameArray should contain the keys realname and username
   * @param array$formatArray contains only realname, username or both as entries
   * @return string formatted name 
   */
  function format_name($nameArray, $formatArray = '') {
    $formatArray = $this->Details->name_format;
    if (is_object(($nameArray)))
      $nameArray = (array) $nameArray;
    if (count($formatArray) == 1)
      return $nameArray[$formatArray[0]];
    else
      return $nameArray[$formatArray[0]] . ' (' . $nameArray[$formatArray[1]] . ')';
  }

  /**
   * Returns the format to be used in a CONCAT addexpression for db_select, example: 
   * $query->addExpression("CONCAT(u.username,' (' ,u.realname, ')')", 'formatted_name');
   * @param type $formatArray
   * @return string CONCAT expression to be used with addExpression of db_select
   */
  function format_name_for_dbselect($formatArray, $table_alias = '') {
    if (count($formatArray) == 1)
      return "CONCAT({$table_alias}.{$formatArray[0]})";
    else
      return "CONCAT({$table_alias}.{$formatArray[0]},' (' ,{$table_alias}.{$formatArray[1]}, ')')";
  }

  /**
   * Gets the balance for each group the current user is a member of. 
   * Stores in $this->AllGroupBalance. 
   * Returns the overview balance list as an array to be used with theme_table
   * @return type array for theme_table
   */
  function get_balance_table($rows_as_link = FALSE) {
    $groups = $this->get_groups();
    $balanceArr = array();
    foreach ($groups as $groupid => $groupdetails) {
      $g = new GD_Group($groupid);
      $userbalance = $g->get_balance_for_user($this->Details->user_id);
      $balanceArr[$groupid]['groupname'] = $g->Details->name;
      $balanceArr[$groupid]['paid'] = $userbalance['expenses_paid'];
      $balanceArr[$groupid]['spent'] = $userbalance['expenses_made'];
      $balanceArr[$groupid]['balance'] = $userbalance['balance'];
    }

    $header = array(
        array('data' => t('Group'), 'field' => 'groupname'),
        array('data' => t('Total paid'), 'field' => 'paid', 'class' => array('r_align')),
        array('data' => t('Total spent'), 'field' => 'spent', 'class' => array('r_align')),
        array('data' => t('Balance'), 'field' => 'balance', 'sort' => 'desc', 'class' => array('r_align'))
    );
    $order = tablesort_get_order($header);
    $sort = tablesort_get_sort($header);
    $sort = $sort == 'asc' ? SORT_ASC : SORT_DESC;
    $this->AllGroupBalance = bs_array_sort($balanceArr, $order['sql'], $sort);

    
    $rows = array();
    foreach ($this->AllGroupBalance as $groupid=>$entry) {
      $rows[] = array(
          array('data' => l($entry['groupname'],'gd/'.$groupid)) ,
          $this->cashval_cell($entry['paid'], 'gd/'.$groupid),
          $this->cashval_cell($entry['spent'], 'gd/'.$groupid),
          $this->cashval_cell($entry['balance'], 'gd/'.$groupid)
      );
    }
    $attributes = array('class' => array('gd'));
    return array('header' => $header, 'attributes' => $attributes, 'rows' => $rows);
  }

  function get_expense_types($return_as_array = FALSE) {
    $fetch = $return_as_array == TRUE ? PDO::FETCH_ASSOC : NULL;
    if (count($this->ExpenseTypes) == 0) {
      gd_connect_db();
      $query = db_select('expense_types', 'e')->fields('e');
      $this->ExpenseTypes = $query->execute()->fetchAllAssoc('expense_type_id', $fetch);
      db_set_active();
    }
    return $this->ExpenseTypes;
  }

  /**
   * Checks if new groups were added in the mobile site
   * If so, forces a menu rebuild
   */
  function check_for_new_groups() {
    gd_connect_db();
    $query = db_select('groups', 'g')->fields('g');
    $query->condition('g.drupal_added', 0, '=');
    $newGroups = $query->execute()->fetchAllAssoc('group_id');
    if (!empty($newGroups)) {
      // only rebuild menu's if yours have changed
      $query = db_select('users_groups', 'ug')->fields('ug');
      $query->condition('ug.user_id', $this->GDid, '=');
      $query->condition('ug.group_id', array_keys($newGroups), 'IN');
      $ownGroups = $query->execute()->fetchAllAssoc('group_id');
      db_set_active();
      if (!empty($ownGroups)) {
        // rebuild menu
        menu_rebuild();
      }
    }
    db_set_active();
  }

}