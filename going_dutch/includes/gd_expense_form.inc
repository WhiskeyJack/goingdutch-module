<?php

function gd_expense_form($form, &$form_state) {
  $groupid = arg(1);
  $group = new GD_Group($groupid);

  // create memberlist
  $memberlist = array();
  foreach ($group->get_members() as $memberid => $details) {
    $memberlist[$memberid] = GDme()->format_name($details);
  }
  natcasesort($memberlist);

  // create event list
  // create from date, for now take 1 month back
  $from = date('Y-m-d', mktime(0, 0, 0, date("m") - 1, date("d"), date("Y")));
  // put in temp array for sorting
  $eventSummaryList = bs_array_sort($group->get_event_list($from), 'event_timestamp');
  $eventlist = array();
  foreach ($eventSummaryList as $eventid => $details) {
    $eventlist[$eventid] = format_date($details->event_timestamp, 'gd_expense_date') . ' - ' . $details->event_name;
  }

  // create type list
  $typelist = array();
  foreach (GDme()->get_expense_types(TRUE) as $id => $desc) {
    $typelist[$id] = t($desc['description']);
  }
  natsort($typelist);

  // Expense by (dropdown)
  $form['gd_expense_owner'] = array(
      '#type' => 'select',
      '#title' => t('Payment made by'),
      '#description' => t('The person that paid this expense.'),
      '#options' => $memberlist,
      '#required' => TRUE,
      '#default_value' => GDme()->Details->user_id,
  );

  // Event (dropdown)
  $eventlist = count($eventlist) > 0 ? $eventlist : $eventlist = array(0 => t('No current events found'));
  $form['gd_expense_event'] = array(
      '#type' => 'select',
      '#title' => t('Event'),
      '#description' => t('Event the payment belongs to.'),
      '#options' => $eventlist,
      '#required' => FALSE,
  );

  // Description
  $form['gd_expense_desc'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#description' => t('Brief description of the payment.'),
      '#size' => 30,
      '#maxlength' => 60,
      '#required' => TRUE,
  );

  // Amount
  $form['gd_expense_amount'] = array(
      '#type' => 'textfield',
      '#title' => t('Amount'),
      //'#description' => t('Host address of the Going Dutch database.'),
      '#size' => 10,
      '#maxlength' => 6,
      '#required' => TRUE,
      '#element_validate' => array('is_formfield_below999with2digits'),
  );

  // Type (dropdown)
  $form['gd_expense_type'] = array(
      '#type' => 'select',
      '#title' => t('Payment type'),
      '#description' => t('The type of payment.'),
      '#options' => $typelist,
      '#required' => FALSE,
  );

  // Expense date
  $form['gd_expense_date'] = array(
      '#type' => 'date_popup',
      '#title' => t('Date'),
      '#description' => t('Date when the payment was made.'),
      '#required' => TRUE,
  );
  
  // Participants
  $form['gd_expense_participants'] = array(
      '#title' => t('Participants'),
      '#description' => t('Who participated in the expense.'),
      '#type' => 'checkboxes',
      '#options' => $memberlist,
      '#multicolumn' => array('width' => 3),
      '#required' => TRUE,
  );

  // Submit
  $form['gd_submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
  );
  return $form;
}

function gd_expense_form_submit($form, &$form_state) {
  $groupid = arg(1);
  $val = $form_state['values'];
  $eventid = empty($val['gd_expense_event']) ? 0 : $val['gd_expense_event'];
  if ($form['gd_expense_date']['#type'] == 'date')
    $date = "{$val['gd_expense_date']['year']}-{$val['gd_expense_date']['month']}-{$val['gd_expense_date']['day']}";
  else 
    $date = $val['gd_expense_date'];
  gd_connect_db();
  $expense_id = db_insert('expenses')->fields(array(
      'type' => $val['gd_expense_type'],
      'user_id' => $val['gd_expense_owner'],
      'group_id' => $groupid,
      'description' => $val['gd_expense_desc'],
      'amount' => $val['gd_expense_amount'],
      'expense_date' => $date,
      'event_id' => $eventid,
      'currency' => '1'
  ))->execute();
  //$expense_id = db_last_insert_id ('expenses', 'expense_id');
  $values = array();
  foreach ($val['gd_expense_participants'] as $memberid) {
    if ($memberid > 0)
      $values[] = array('user_id' => $memberid, 'expense_id' => $expense_id);
  }
  $query = db_insert('users_expenses')->fields(array('user_id', 'expense_id'));
  foreach ($values as $record) {
    $query->values($record);
  }
  $query->execute();
  db_set_active();
  drupal_set_message(t('Payment has been recorded'));
}

function is_formfield_below999with2digits($element, &$form_state) {
  if (empty($element['#value']))
    return;
  if (!(preg_match("/^\d{1,3}([\.,]\d{1,2})?$/D", $element['#value'])))
    form_error($element, t('The "!name" entered is not valid', array('!name' => t($element['#title']))));
  else {
    if (strpos($element['#value'], ',')) {
      // comma found, make it a dot
      form_set_value($element, str_replace(',', '.', $element['#value']), $form_state);
    }
  }
}