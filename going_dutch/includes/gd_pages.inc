<?php

/**
 * Creates the html for overview page with the balance table for all groups
 * @global type $user
 * @return type 
 */
function gd_show_overview() {
  drupal_add_css(drupal_get_path('module', 'going_dutch') . '/css/going_dutch.css');
  global $user;
  if ($user->uid == 0) {
    $welcome = variable_get('gd_welcome_page');
    if (!empty($welcome) && menu_get_item($welcome) != FALSE)
      drupal_goto($welcome);
    else {
      drupal_set_title(t("Welkom"));
      $html = '<p>' . t('Welcome to Going Dutch!') . '</p>';
    }
  } else {
    drupal_set_title(t("Overview of  your groups"));
    $balance_table = GDme()->get_balance_table();
    $html = theme('table', $balance_table);
  }
  return $html;
}

/**
 * Temporary function to output the actual html of a group page
 * @param type $gd_group_id
 * @param type $gd_group_page
 * @param type $options
 * @return type 
 */
function gd_show_group($gd_group_id, $gd_group_page = '', $options = '') {
  drupal_add_css(drupal_get_path('module', 'going_dutch') . '/css/going_dutch.css');

  if (empty($gd_group_page))
    drupal_goto("gd/{$gd_group_id}/balance");

  global $user;

  switch ($gd_group_page) {
    case "balance":
      $group = new GD_Group($gd_group_id);
      $balance_table = $group->get_balance_table($user->gd);
      drupal_set_title(t("Balance sheet for @groupname", array('@groupname' => $group->Details->name)));
      $html = theme('table', $balance_table);
      break;

    case "payment":
      drupal_set_title(t("Enter a payment"));
      module_load_include('inc', 'going_dutch', 'includes/gd_expense_form');
      $html = drupal_get_form('gd_expense_form');
      break;

    case "deposit":
      module_load_include('inc', 'going_dutch', 'includes/gd_deposit_form');
      $html = drupal_get_form('gd_deposit_form');
      break;

    case "event":
      drupal_set_title(t("Add an event"));
      module_load_include('inc', 'going_dutch', 'includes/gd_event_form');
      $html = drupal_get_form('gd_event_form');
      break;

    case "events":
      $group = new GD_Group($gd_group_id);
      drupal_set_title(t("Event summary for @groupname", array('@groupname' => $group->Details->name)));
      $event_summary_table = $group->get_events_summary_table($user->gd);
      $html = theme('table', $event_summary_table);
      $html .= theme('pager', array('tags' => array()));
      break;

    case "expenses":
      $group = new GD_Group($gd_group_id);
      drupal_set_title(t("Expense list for @groupname", array('@groupname' => $group->Details->name)));
      $expenses_table = $group->get_expenses_table($user->gd);
      $html = theme('table', $expenses_table);
      $html .= theme('pager', array('tags' => array()));
      break;

    case "deposits":
      $group = new GD_Group($gd_group_id);
      drupal_set_title(t("Deposit list for @groupname", array('@groupname' => $group->Details->name)));
      $deposits_table = $group->get_deposits_table($user->gd);
      $html = theme('table', $deposits_table);
      $html .= theme('pager', array('tags' => array()));
      break;

    case "visual":
      $group = new GD_Group($gd_group_id);
      drupal_set_title(t("Graph for @groupname", array('@groupname' => $group->Details->name)));
      $html = gd_get_graph_page($gd_group_id, $gd_group_page, $options);
      break;

    case "settle":
      $group = new GD_Group($gd_group_id);
      drupal_set_title(t("Settle transactions for @groupname", array('@groupname' => $group->Details->name)));
      $settle_table = gd_get_settle_table($group);
      $table = theme('table', $settle_table);
      $html = '<p>' . t('This table shows the transactions necessary to even out all balances, making the group balance effectively zero.') . '</p>';
      $html .= $table;
      break;

    default:
      $html = "<p>Page for group {$gd_group_id} {$gd_group_page} </p>";
  }

  return $html;
}

function m_convert($m) {
  return mb_convert_encoding($m[1], "UTF-8", "HTML-ENTITIES");
}

function gd_get_graph_page($gd_group_id, $gd_group_page = '', $options = '') {
  global $user;

  $group = new GD_Group($gd_group_id);
  $group->get_expenses($user->gd, TRUE);
  $group->get_deposits_table($user->gd, TRUE);
  $group->fetch_members();

  $nf = GDme()->Details->number_format;
  $input = $nf->currency_symbol;
  //$symbol = preg_replace_callback("/(&#[0-9]+;)/", function($m) {
  //          return mb_convert_encoding($m[1], "UTF-8", "HTML-ENTITIES");
  //        }, $input);
  $symbol = preg_replace_callback("/(&#[0-9]+;)/", "m_convert", $input);

  $tooltipFormatter = "function() {
                var s = Highcharts.dateFormat('%A, %b %e, %Y', this.x) + '<br /><b>' + this.points[0].point.name + '</b>';
                $.each(this.points, function(i, point) {
                    s += '<br/>' + '<span style=\"color:'+point.series.color+';\">'  
                              + point.series.name +': ' + '</span>' 
                              + '{$symbol} ' + Highcharts.numberFormat(point.y, '{$nf->decimal_count}', '{$nf->decimal_sep}', '{$nf->thousand_sep}')
                });
                return s;}";

  $chart = new Highstock_lib();
  $chart->set_graph_type('line')
          ->display_shadow(TRUE)
          ->set_graph_title('')
          ->set_yAxis(array(), 'Balance')
          ->set_xAxis(false, false, 'datetime')
          ->set_theme('grey')
          ->set_tooltip_options('formatter', $tooltipFormatter)
          ->set_number_format(GDme()->Details->number_format->decimal_sep, GDme()->Details->number_format->thousand_sep, 2);


  foreach ($group->get_members() as $memberid => $memberdetails) {
    $chart->add_serie_with_key(GDme()->format_name($memberdetails), array(), '', $memberid);
    $balance[$memberid] = 0;
  }

  $dc = GDme()->Details->number_format->decimal_count;
  foreach ($group->Expenses as $expense) {
    $timestamp = strtotime($expense->expense_date);
    $balance[$expense->user_id] += $expense->amount;
    $amount = number_format($expense->amount, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep);
    foreach (explode(',', $expense->expense_user_ids) as $memberid) {
      $balance[$memberid] -= $expense->amount_per_person;
      $chart->push_serie_data_with_x_by_key($memberid, round($balance[$memberid], $dc), $timestamp, TRUE, "{$expense->description}: {$symbol} {$amount}");
    }
  }
  foreach ($group->Deposits as $deposit) {
    $timestamp = strtotime($deposit->expense_date);
    $balance[$deposit->deposit_holder_id] -= $deposit->amount;
    $amount = number_format($deposit->amount, $nf->decimal_count, $nf->decimal_sep, $nf->thousand_sep);
    foreach (explode(',', $deposit->expense_user_ids) as $memberid) {
      $balance[$memberid] += $deposit->amount_per_person;
      $chart->push_serie_data_with_x_by_key($memberid, round($balance[$memberid], $dc), $timestamp, TRUE, "{$deposit->description} (" . t("deposit") . ") {$symbol} {$amount}");
    }
  }

  $s_graph = $chart->render();

  drupal_set_title(t("Balance visual for @groupname", array('@groupname' => $group->Details->name)));
  $html = "";
  $html .= $s_graph;
  return $html;
}

/**
 * Calculates the transactions necessary to even out all balances
 * Based on old (ugly) code, needs a rewrite
 * @param type $group
 * @return type 
 */
function gd_get_settle_table($group) {
  module_load_include('inc', 'going_dutch', 'includes/gd_settle_function_old');
  $balance = $group->get_balance();
  $i = 0;
  foreach ($balance as $memberid => $detail) {
    $listarray[$i]['name'] = $detail['formattedname'];
    $listarray[$i]['balance'] = $detail['balance'];
    $listarray[$i]['user_id'] = $memberid;
    $i++;
  }
  $settle_array = close_group_expenses($listarray);

  $headers = array(
      array('data' => t('Payer'), 'field' => 'pay_name'),
      array('data' => t('Recipient'), 'field' => 'get_name'),
      array('data' => t('Amount'), 'field' => 'sum', 'class' => array('r_align')),
  );

  $order = tablesort_get_order($headers);
  $sort = tablesort_get_sort($headers);
  $sort = $sort == 'asc' ? SORT_ASC : SORT_DESC;
  $settle_array = bs_array_sort($settle_array, $order['sql'], $sort);
  $rows = array();
  foreach ($settle_array as $entry) {
    $rows[] = array(
        array('data' => $entry['pay_name']),
        array('data' => $entry['get_name']),
        GDme()->cashval_cell($entry['sum']),
    );
  }
  $attributes = array('class' => array('gd'));
  return array('header' => $headers, 'attributes' => $attributes, 'rows' => $rows);
}
